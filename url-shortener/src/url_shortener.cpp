#include "url_shortener.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <chrono>
#include <sstream>
#include <arpa/inet.h>
#include <netdb.h>

using namespace std;

void sendStatisticSimple(const string& ip_address, const string& url, const string& short_code, long timestamp) {
    thread([ip_address, url, short_code, timestamp]() {
        try {
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) {
                cerr << "Failed to create socket for statistics" << endl;
                return;
            }
            
            sockaddr_in server_addr;
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(8081);

            struct hostent* host = gethostbyname("statistics_service");
            if (host == nullptr) {
                cerr << "Failed to resolve statistics_service hostname" << endl;
                close(sock);
                return;
            }
            memcpy(&server_addr.sin_addr, host->h_addr_list[0], host->h_length);

            if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                cerr << "Failed to connect to statistics service" << endl;
                close(sock);
                return;
            }

            ostringstream json;
            json << "{";
            json << "\"ip_address\": \"" << ip_address << "\", ";
            json << "\"url\": \"" << url << "\", ";
            json << "\"short_code\": \"" << short_code << "\", ";
            json << "\"timestamp\": " << timestamp;
            json << "}";
            
            string json_data = json.str();

            ostringstream http_request;
            http_request << "POST / HTTP/1.1\r\n";
            http_request << "Host: statistics_service:8081\r\n";
            http_request << "Content-Type: application/json\r\n";
            http_request << "Content-Length: " << json_data.length() << "\r\n";
            http_request << "Connection: close\r\n";
            http_request << "\r\n";
            http_request << json_data;
            
            string request = http_request.str();

            send(sock, request.c_str(), request.length(), 0);

            char buffer[1024];
            recv(sock, buffer, sizeof(buffer), 0);
            
            close(sock);
            cout << "Statistic sent to statistics service: " << short_code << " from " << ip_address << endl;
            
        } catch (const exception& e) {
            cerr << "Error sending statistic: " << e.what() << endl;
        }
    }).detach();
}

string extractClientIP(int client_socket) {
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    getpeername(client_socket, (struct sockaddr*)&addr, &addr_size);
    return inet_ntoa(addr.sin_addr);
}

URLShortener::URLShortener(const string& db_host, int db_port) 
    : db(db_host, db_port) {
    
    if (!db.connect()) {
        throw runtime_error("Failed to connect to database");
    }

    try {
        string response = db.sendCommand("PING");
        cout << "PING response: '" << response << "'" << endl;
        if (response != "+PONG" && response != "PONG") {
            throw runtime_error("Database ping failed. Got: " + response);
        }
    } catch (const exception& e) {
        throw runtime_error(string("Database connection test failed: ") + e.what());
    }
}

URLShortener::~URLShortener() {
}

string URLShortener::generateShortCode() {
    static random_device rd;
    static mt19937 gen(rd());
    static uniform_int_distribution<> type_dis(0, 2);
    static uniform_int_distribution<> digit_dis(48, 57);
    static uniform_int_distribution<> upper_dis(65, 90);
    static uniform_int_distribution<> lower_dis(97, 122);
    
    string code;
    for (int i = 0; i < 6; i++) {
        int type = type_dis(gen);
        char c;
        
        switch (type) {
            case 0:  c = static_cast<char>(digit_dis(gen)); break;
            case 1:  c = static_cast<char>(upper_dis(gen)); break;
            case 2:  c = static_cast<char>(lower_dis(gen)); break;
            default: c = '0';
        }
        
        code += c;
    }
    
    return code;
}

string URLShortener::extractPath(const string& request) {
    size_t start = request.find(' ') + 1;
    size_t end = request.find(' ', start);
    return request.substr(start, end - start);
}

string URLShortener::extractBody(const string& request) {
    size_t pos = request.find("\r\n\r\n");
    if (pos != string::npos) {
        return request.substr(pos + 4);
    }
    return "";
}

string URLShortener::createResponse(int status_code, const string& body, const string& content_type) {
    string status_text;
    switch (status_code) {
        case 200: status_text = "OK"; break;
        case 201: status_text = "Created"; break;
        case 302: status_text = "Found"; break;
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

void URLShortener::handleClient(int client_socket) {
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        close(client_socket);
        return;
    }
    
    string request(buffer, bytes_received);
    string path = extractPath(request);
    
    cout << "Request: " << path << endl;
    
    string response;
    
    try {
        if (request.find("POST /shorten") == 0) {
            string body = extractBody(request);

            size_t url_pos = body.find("url=");
            if (url_pos == string::npos) {
                response = createResponse(400, "Bad Request: Missing URL");
            } else {
                string long_url = body.substr(url_pos + 4);

                long_url.erase(long_url.find_last_not_of(" \n\r\t") + 1);

                string short_code = generateShortCode();

                string cmd = "HSET urls " + short_code + " " + long_url;
                string db_response = db.sendCommand(cmd);
                
                if (db_response == "OK") {
                    string result = "Short URL: http://localhost:8080/" + short_code + "\n";
                    response = createResponse(201, result);
                    
                    cout << "Created: " << short_code << " -> " << long_url << endl;
                } else {
                    response = createResponse(500, "Failed to create short URL");
                }
            }
            
        } else if (request.find("GET /") == 0 && path.length() > 1) {
            string short_code = path.substr(1);

            string cmd = "HGET urls " + short_code;
            string long_url = db.sendCommand(cmd);
            
            if (long_url != "(nil)" && !long_url.empty()) {
                string client_ip = extractClientIP(client_socket);

                auto now = chrono::system_clock::now();
                auto timestamp = chrono::duration_cast<chrono::seconds>(now.time_since_epoch()).count();

                sendStatisticSimple(client_ip, long_url, short_code, timestamp);

                ostringstream redirect_response;
                redirect_response << "HTTP/1.1 302 Found\r\n";
                redirect_response << "Location: " << long_url << "\r\n";
                redirect_response << "Connection: close\r\n";
                redirect_response << "\r\n";
                response = redirect_response.str();
                
                cout << "Redirect: " << short_code << " -> " << long_url << " from IP: " << client_ip << endl;
            } else {
                response = createResponse(404, "Short URL not found");
            }
            
        } else if (path == "/" || path == "") {

            string html = "<html><body><h1>URL Shortener Service</h1>"
                          "<p>Use POST /shorten to create short URLs</p>"
                          "<p>Use GET /{code} to redirect</p></body></html>";
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

void URLShortener::start(int port) {
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
    
    cout << "URL Shortener running on port " << port << endl;
    cout << "Endpoints:" << endl;
    cout << "  POST /shorten - Create short URL" << endl;
    cout << "  GET /<code>   - Redirect to original URL (and send statistics)" << endl;
    cout << "  GET /         - Info page" << endl;

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            cerr << "Failed to accept connection" << endl;
            continue;
        }
        
        thread(&URLShortener::handleClient, this, client_socket).detach();
    }
    
    close(server_socket);
}
