#ifndef URL_SHORTENER_H
#define URL_SHORTENER_H

#include "db_client.h"
#include <string>
#include <random>
#include <sstream>

using namespace std;

class URLShortener {
private:
    DBClient db;
    
    string generateShortCode();
    string extractPath(const string& request);
    string extractBody(const string& request);
    string createResponse(int status_code, const string& body, const string& content_type = "text/plain");
    
public:
    URLShortener(const string& db_host, int db_port);
    ~URLShortener();
    void start(int port);
    void handleClient(int client_socket);
};

#endif
