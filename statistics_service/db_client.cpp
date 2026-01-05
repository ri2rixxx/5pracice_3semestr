#include "db_client.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <netdb.h>

using namespace std;

DBClient::DBClient(const string& host, int port) 
    : host_(host), port_(port), socket_fd_(-1), connected_(false) {}

DBClient::~DBClient() {
    disconnect();
}

bool DBClient::connect() {
    if (connected_) return true;
    
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        cerr << "Failed to create socket" << endl;
        return false;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);

    if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) {

        struct hostent* host = gethostbyname(host_.c_str());
        if (host == nullptr) {
            cerr << "Failed to resolve host: " << host_ << endl;
            close(socket_fd_);
            socket_fd_ = -1;
            return false;
        }
        memcpy(&server_addr.sin_addr, host->h_addr_list[0], host->h_length);
    }

    cout << "Connecting to " << host_ << ":" << port_ << endl;

    int attempts = 0;
    const int max_attempts = 20;
    
    while (attempts < max_attempts) {
        if (::connect(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) == 0) {
            connected_ = true;
            cout << "Connected successfully!" << endl;
            return true;
        }
        
        attempts++;
        if (attempts < max_attempts) {
            cout << "Connection attempt " << attempts << " failed. Retrying in 1 second..." << endl;
            sleep(1);
        }
    }

    cerr << "Connection failed after " << max_attempts << " attempts" << endl;
    close(socket_fd_);
    socket_fd_ = -1;
    return false;
}

void DBClient::disconnect() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
    connected_ = false;
}

bool DBClient::isConnected() const {
    return connected_;
}

string DBClient::sendCommand(const string& command) {
    if (!connected_ && !connect()) {
        return "ERROR: Not connected";
    }

    string cmd = command + "\r\n";
    
    cout << "Sending command: " << cmd;
    
    if (send(socket_fd_, cmd.c_str(), cmd.length(), 0) < 0) {
        connected_ = false;
        return "ERROR: Send failed";
    }

    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    
    int bytes_read = recv(socket_fd_, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read < 0) {
        connected_ = false;
        return "ERROR: Receive failed";
    }

    string response(buffer, bytes_read);
    cout << "Received response: " << response << endl;

    if (!response.empty() && response.back() == '\n') {
        response.pop_back();
    }
    if (!response.empty() && response.back() == '\r') {
        response.pop_back();
    }

    return response;
}
