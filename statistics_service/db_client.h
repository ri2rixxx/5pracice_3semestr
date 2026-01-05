#ifndef DB_CLIENT_H
#define DB_CLIENT_H

#include <string>

class DBClient {
private:
    std::string host_;
    int port_;
    int socket_fd_;
    bool connected_;

public:
    DBClient(const std::string& host, int port);
    ~DBClient();
    
    bool connect();
    void disconnect();
    bool isConnected() const;
    std::string sendCommand(const std::string& command);
};

#endif
