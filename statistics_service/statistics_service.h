#ifndef STATISTICS_SERVICE_H
#define STATISTICS_SERVICE_H

#include "db_client.h"
#include <string>
#include <sstream>
#include <vector>
#include <map>

using namespace std;

struct ReportItem {
    int id;
    int pid;
    string url;
    string source_ip;
    string time_interval;
    int count;
};

class StatisticsService {
private:
    DBClient db;
    
    string extractPath(const string& request);
    string extractBody(const string& request);
    string createResponse(int status_code, const string& body, const string& content_type = "text/plain");
    
    void saveStatistic(const string& ip_address, const string& url, const string& short_code, long timestamp);
    vector<ReportItem> generateReport(const vector<string>& dimensions);
    
public:
    StatisticsService(const string& db_host, int db_port);
    void start(int port);
    void handleClient(int client_socket);
};

#endif
