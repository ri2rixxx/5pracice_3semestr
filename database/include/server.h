#ifndef SERVER_H
#define SERVER_H

#include <string>
#include "database.h"

class Server {
private:
    int port;
    int server_fd;
    Database db;
    bool running;

public:
    Server(int port);
    ~Server();
    void start();
    void stop();
    void handleClient(int client_socket);
};

#endif
