#include "statistics_service.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <vector>
#include <map>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

using namespace std;

map<string, string> parseSimpleJSON(const string& json_str) {
    map<string, string> result;
    stringstream ss(json_str);
    char ch;
    
    ss >> ch;
    
    while (ss >> ch && ch != '}') {
        if (ch == '"') {
            string key;
            getline(ss, key, '"');
            
            ss >> ch;
            ss >> ch;
            
            string value;
            if (ch == '"') {
                getline(ss, value, '"');
            } else {
                ss.putback(ch);
                ss >> value;
                if (!value.empty() && value.back() == ',') {
                    value.pop_back();
                }
            }
            
            result[key] = value;

            if (ss.peek() == ',') ss.get();
        }
    }
    
    return result;
}

string createSimpleJSON(const map<string, string>& data) {
    ostringstream json;
    json << "{";
    
    bool first = true;
    for (const auto& [key, value] : data) {
        if (!first) json << ", ";
        json << "\"" << key << "\": ";

        bool is_number = !value.empty() && 
                        (isdigit(value[0]) || (value[0] == '-' && value.size() > 1 && isdigit(value[1])));
        
        if (is_number) {
            json << value;
        } else if (value == "null") {
            json << "null";
        } else {
            json << "\"" << value << "\"";
        }
        
        first = false;
    }
    
    json << "}";
    return json.str();
}

StatisticsService::StatisticsService(const string& db_host, int db_port) 
    : db(db_host, db_port) {
    
    if (!db.connect()) {
        throw runtime_error("Failed to connect to database");
    }
    
    try {
        string response = db.sendCommand("PING");
        if (response != "+PONG" && response != "PONG") {
            throw runtime_error("Database ping failed");
        }
    } catch (const exception& e) {
        throw runtime_error(string("Database connection test failed: ") + e.what());
    }
}

string StatisticsService::extractPath(const string& request) {
    size_t start = request.find(' ') + 1;
    size_t end = request.find(' ', start);
    return request.substr(start, end - start);
}

string StatisticsService::extractBody(const string& request) {
    size_t pos = request.find("\r\n\r\n");
    if (pos != string::npos) {
        return request.substr(pos + 4);
    }
    return "";
}

string StatisticsService::createResponse(int status_code, const string& body, const string& content_type) {
    string status_text;
    switch (status_code) {
        case 200: status_text = "OK"; break;
        case 201: status_text = "Created"; break;
        case 400: status_text = "Bad Request"; break;
        case 404: status_text = "Not Found"; break;
        case 500: status_text = "Internal Server Error"; break;
        default: status_text = "Unknown"; break;
    }
    
    ostringstream response;
    response << "HTTP/1.1 " << status_code << " " << status_text << "\r\n";
    response << "Content-Type: " << content_type << "\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;
    
    return response.str();
}

vector<string> parseJSONArray(const string& json_str) {
    vector<string> result;
    stringstream ss(json_str);
    char ch;
    
    ss >> ch;
    
    while (ss >> ch && ch != ']') {
        if (ch == '"') {
            string item;
            getline(ss, item, '"');
            result.push_back(item);

            if (ss.peek() == ',') ss.get();
        }
    }
    
    return result;
}

string createJSONArray(const vector<string>& items) {
    ostringstream json;
    json << "[";
    
    for (size_t i = 0; i < items.size(); i++) {
        if (i > 0) json << ", ";
        json << "\"" << items[i] << "\"";
    }
    
    json << "]";
    return json.str();
}

void StatisticsService::saveStatistic(const string& ip_address, const string& url, const string& short_code, long timestamp) {
    try {

        string key = "stats:" + to_string(timestamp) + ":" + ip_address + ":" + short_code;
        db.sendCommand("SET " + key + " " + url);

        db.sendCommand("SADD stats_urls " + short_code);
        db.sendCommand("SADD stats_ips " + ip_address);
        
        cout << "Statistic saved: " << key << " -> " << url << endl;
        
    } catch (const exception& e) {
        cerr << "Error saving statistic: " << e.what() << endl;
    }
}

string getTimeInterval(long timestamp) {
    time_t time_val = timestamp;
    tm* tm_time = localtime(&time_val);
    
    int hour = tm_time->tm_hour;
    int min = tm_time->tm_min;

    int next_hour = hour;
    int next_min = min + 1;
    
    if (next_min >= 60) {
        next_min = 0;
        next_hour = (hour + 1) % 24;
    }
    
    ostringstream oss;
    oss << setw(2) << setfill('0') << hour << ":" 
        << setw(2) << setfill('0') << min << "-"
        << setw(2) << setfill('0') << next_hour << ":"
        << setw(2) << setfill('0') << next_min;
    
    return oss.str();
}

vector<ReportItem> StatisticsService::generateReport(const vector<string>& dimensions) {
    vector<ReportItem> report;
    int id_counter = 1;

    string response = db.sendCommand("KEYS stats:*");
    istringstream iss(response);
    vector<string> keys;
    string key;
    
    while (getline(iss, key)) {
        if (!key.empty() && key != "(empty)") {
            keys.push_back(key);
        }
    }
    
    if (keys.empty()) {
        return report;
    }

    map<string, map<string, map<string, int>>> grouped_data;
    
    for (const auto& full_key : keys) {

        string key = full_key;

        if (key.back() == '\n') key.pop_back();
        if (key.back() == '\r') key.pop_back();
        
        vector<string> parts;
        stringstream ss(key);
        string part;
        
        while (getline(ss, part, ':')) {
            parts.push_back(part);
        }
        
        if (parts.size() >= 4 && parts[0] == "stats") {
            long timestamp;
            try {
                timestamp = stol(parts[1]);
            } catch (...) {
                continue;
            }
            
            string ip = parts[2];
            string short_code = parts[3];

            string url_value = db.sendCommand("GET " + key);
            if (url_value.empty() || url_value == "(nil)" || url_value.find("ERROR") != string::npos) {
                continue;
            }

            string full_url = db.sendCommand("HGET urls " + short_code);
            if (full_url.empty() || full_url == "(nil)" || full_url.find("ERROR") != string::npos) {
                continue;
            }
            
            string url_with_code = full_url + " (" + short_code + ")";
            string time_interval = getTimeInterval(timestamp);

            string dim1, dim2, dim3;
            
            for (size_t i = 0; i < dimensions.size(); i++) {
                if (dimensions[i] == "URL") {
                    if (i == 0) dim1 = url_with_code;
                    else if (i == 1) dim2 = url_with_code;
                    else if (i == 2) dim3 = url_with_code;
                } else if (dimensions[i] == "SourceIP") {
                    if (i == 0) dim1 = ip;
                    else if (i == 1) dim2 = ip;
                    else if (i == 2) dim3 = ip;
                } else if (dimensions[i] == "TimeInterval") {
                    if (i == 0) dim1 = time_interval;
                    else if (i == 1) dim2 = time_interval;
                    else if (i == 2) dim3 = time_interval;
                }
            }

            if (dimensions.size() == 3) {
                grouped_data[dim1][dim2][dim3]++;
            } else if (dimensions.size() == 2) {
                grouped_data[dim1][dim2]["__count__"]++;
            } else if (dimensions.size() == 1) {
                grouped_data[dim1]["__count__"]["__count__"]++;
            }
        }
    }
    
    // Создаем иерархический отчет
    if (dimensions.size() >= 1) {
        int parent_id = 0;
        int level1_id = 0;
        
        // Первый уровень
        for (const auto& [dim1, submap1] : grouped_data) {
            ReportItem item1;
            item1.id = id_counter++;
            item1.pid = 0;
            item1.count = 0;
            
            // Устанавливаем значения для первого уровня
            if (dimensions[0] == "URL") item1.url = dim1;
            else if (dimensions[0] == "SourceIP") item1.source_ip = dim1;
            else if (dimensions[0] == "TimeInterval") item1.time_interval = dim1;
            
            // Считаем общее количество для этого уровня
            for (const auto& [dim2, submap2] : submap1) {
                for (const auto& [dim3, count] : submap2) {
                    item1.count += count;
                }
            }
            
            report.push_back(item1);
            level1_id = item1.id;
            
            // Второй уровень
            if (dimensions.size() >= 2) {
                for (const auto& [dim2, submap2] : submap1) {
                    ReportItem item2;
                    item2.id = id_counter++;
                    item2.pid = level1_id;
                    item2.count = 0;
                    
                    // Устанавливаем значения для второго уровня
                    if (dimensions[1] == "URL") item2.url = dim2;
                    else if (dimensions[1] == "SourceIP") item2.source_ip = dim2;
                    else if (dimensions[1] == "TimeInterval") item2.time_interval = dim2;
                    
                    // Считаем количество для второго уровня
                    for (const auto& [dim3, count] : submap2) {
                        item2.count += count;
                    }
                    
                    report.push_back(item2);
                    int level2_id = item2.id;
                    
                    // Третий уровень
                    if (dimensions.size() >= 3 && dim2 != "__count__") {
                        for (const auto& [dim3, count] : submap2) {
                            if (dim3 != "__count__") {
                                ReportItem item3;
                                item3.id = id_counter++;
                                item3.pid = level2_id;
                                item3.count = count;
                                
                                // Устанавливаем значения для третьего уровня
                                if (dimensions[2] == "URL") item3.url = dim3;
                                else if (dimensions[2] == "SourceIP") item3.source_ip = dim3;
                                else if (dimensions[2] == "TimeInterval") item3.time_interval = dim3;
                                
                                report.push_back(item3);
                            }
                        }
                    }
                }
            }
        }
    }
    
    return report;
}

string reportToJSON(const vector<ReportItem>& report) {
    ostringstream json;
    json << "[";
    
    for (size_t i = 0; i < report.size(); i++) {
        if (i > 0) json << ", ";
        
        json << "{";
        json << "\"Id\": " << report[i].id << ", ";
        
        if (report[i].pid == 0) {
            json << "\"Pid\": null, ";
        } else {
            json << "\"Pid\": " << report[i].pid << ", ";
        }
        
        if (report[i].url.empty()) {
            json << "\"URL\": null, ";
        } else {
            json << "\"URL\": \"" << report[i].url << "\", ";
        }
        
        if (report[i].source_ip.empty()) {
            json << "\"SourceIP\": null, ";
        } else {
            json << "\"SourceIP\": \"" << report[i].source_ip << "\", ";
        }
        
        if (report[i].time_interval.empty()) {
            json << "\"TimeInterval\": null, ";
        } else {
            json << "\"TimeInterval\": \"" << report[i].time_interval << "\", ";
        }
        
        json << "\"Count\": " << report[i].count;
        json << "}";
    }
    
    json << "]";
    return json.str();
}

void StatisticsService::handleClient(int client_socket) {
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        close(client_socket);
        return;
    }
    
    string request(buffer, bytes_received);
    string method = request.substr(0, request.find(' '));
    string path = extractPath(request);
    
    cout << "Statistics request: " << method << " " << path << endl;
    
    string response;
    
    try {
        if (method == "POST" && path == "/") {

            string body = extractBody(request);
            
            if (!body.empty()) {
                try {
                    auto data = parseSimpleJSON(body);
                    
                    if (data.count("ip_address") && data.count("url") && 
                        data.count("short_code") && data.count("timestamp")) {
                        
                        long timestamp;
                        try {
                            timestamp = stol(data["timestamp"]);
                        } catch (...) {
                            timestamp = time(nullptr);
                        }
                        
                        saveStatistic(data["ip_address"], data["url"], data["short_code"], timestamp);
                        response = createResponse(201, "Statistic saved");
                    } else {
                        response = createResponse(400, "Missing required fields");
                    }
                } catch (const exception& e) {
                    response = createResponse(400, string("Bad JSON: ") + e.what());
                }
            } else {
                response = createResponse(400, "Empty request body");
            }
            
        } else if (method == "POST" && path == "/report") {

            string body = extractBody(request);
            
            if (!body.empty()) {
                try {

                    size_t dim_pos = body.find("\"Dimensions\"");
                    if (dim_pos == string::npos) {
                        response = createResponse(400, "{\"error\": \"Dimensions array is required\"}", "application/json");
                    } else {

                        size_t array_start = body.find('[', dim_pos);
                        size_t array_end = body.find(']', array_start);
                        
                        if (array_start == string::npos || array_end == string::npos) {
                            response = createResponse(400, "{\"error\": \"Invalid Dimensions array\"}", "application/json");
                        } else {
                            string array_str = body.substr(array_start, array_end - array_start + 1);
                            vector<string> dimensions = parseJSONArray(array_str);

                            bool valid = true;
                            for (const auto& dim : dimensions) {
                                if (dim != "URL" && dim != "SourceIP" && dim != "TimeInterval") {
                                    response = createResponse(400, "{\"error\": \"Invalid dimension: " + dim + "\"}", "application/json");
                                    valid = false;
                                    break;
                                }
                            }
                            
                            if (valid) {
                                vector<ReportItem> report = generateReport(dimensions);
                                string json_report = reportToJSON(report);
                                response = createResponse(200, json_report, "application/json");
                            }
                        }
                    }
                } catch (const exception& e) {
                    response = createResponse(400, string("{\"error\": \"") + e.what() + "\"}", "application/json");
                }
            } else {
                response = createResponse(400, "{\"error\": \"Empty request body\"}", "application/json");
            }
            
        } else if (method == "GET" && (path == "/" || path == "")) {

            string html = "<html><body><h1>Statistics Service</h1>"
                          "<p>Endpoints:</p>"
                          "<ul>"
                          "<li>POST / - Receive statistics (JSON)</li>"
                          "<li>POST /report - Generate report (JSON)</li>"
                          "</ul></body></html>";
            response = createResponse(200, html, "text/html");
            
        } else {
            response = createResponse(404, "Not Found");
        }
        
    } catch (const exception& e) {
        cerr << "Error handling request: " << e.what() << endl;
        response = createResponse(500, string("Internal Server Error: ") + e.what());
    }
    
    send(client_socket, response.c_str(), response.length(), 0);
    close(client_socket);
}

void StatisticsService::start(int port) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        throw runtime_error("Failed to create socket");
    }
    
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(server_socket);
        throw runtime_error("Failed to bind socket");
    }
    
    if (listen(server_socket, 10) < 0) {
        close(server_socket);
        throw runtime_error("Failed to listen on socket");
    }
    
    cout << "Statistics Service running on port " << port << endl;
    cout << "Endpoints:" << endl;
    cout << "  POST /      - Receive statistics" << endl;
    cout << "  POST /report - Generate report with dimensions" << endl;
    cout << "  GET /       - Info page" << endl;
    
    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            cerr << "Failed to accept connection" << endl;
            continue;
        }
        
        thread(&StatisticsService::handleClient, this, client_socket).detach();
    }
    
    close(server_socket);
}
