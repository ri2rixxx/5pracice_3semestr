#include "database.h"
#include "command_processor.h"
#include "server.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>

using namespace std;

Database::Database(const string& filename) : db_file(filename) {
    loadFromFile();
}

Database::~Database() {
    saveToFile();
    for (auto& pair : hash_tables) {
        freeHashTable(*pair.second);
        delete pair.second;
    }
    for (auto& pair : sets) {
        freeSet(*pair.second);
        delete pair.second;
    }
    for (auto& pair : stacks) {
        freeStack(*pair.second);
        delete pair.second;
    }
    for (auto& pair : queues) {
        freeQueue(*pair.second);
        delete pair.second;
    }
}

void Database::loadFromFile() {
    ifstream file(db_file);
    if (!file.is_open()) return;

    string line;
    while (getline(file, line)) {
        size_t pos = line.find('=');
        if (pos != string::npos) {
            string key = line.substr(0, pos);
            string value = line.substr(pos + 1);
            simple_storage[key] = value;
        }
    }
    file.close();
}

void Database::saveToFile() {
    ofstream file(db_file);
    if (!file.is_open()) return;

    for (const auto& pair : simple_storage) {
        file << pair.first << "=" << pair.second << "\n";
    }
    file.close();
}

Server::Server(int port) : port(port), db("database.db"), running(false) { 
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        throw runtime_error("Не удалось создать сокет");
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        close(server_fd);
        throw runtime_error("Ошибка bind");
    }

    listen(server_fd, 10);
}

Server::~Server() {
    stop();
    if (server_fd >= 0) {
        close(server_fd);
    }
}

void Server::start() {
    running = true;
    cout << "Сервер запущен на порту " << port << endl;

    while (running) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

        if (client_socket >= 0) {
            thread(&Server::handleClient, this, client_socket).detach();
        }
    }
}

void Server::stop() {
    running = false;
}

void Server::handleClient(int client_socket) {
    char buffer[4096];
    
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_read <= 0) break;

        string command(buffer);

        if (command.back() == '\n') command.pop_back();
        if (command.back() == '\r') command.pop_back();
        
        string response = processCommand(db, command) + "\n";
        
        send(client_socket, response.c_str(), response.length(), 0);
    }
    
    close(client_socket);
}
