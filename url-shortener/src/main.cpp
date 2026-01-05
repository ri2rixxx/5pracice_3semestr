#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <csignal>
#include "url_shortener.h"

using namespace std;

volatile bool running = true;

void signalHandler(int signal) {
    cout << "\nReceived signal " << signal << ", shutting down" << endl;
    running = false;
}

int main() {
    std::cout << "URL Shortener Service" << std::endl;
    
    const char* db_host = std::getenv("DB_HOST");
    const char* db_port_str = std::getenv("DB_PORT");
    
    if (!db_host) db_host = "127.0.0.1";
    int db_port = db_port_str ? std::atoi(db_port_str) : 6379;
    
    std::cout << "DB Host: " << db_host << std::endl;
    std::cout << "DB Port: " << db_port << std::endl;

    try {
        URLShortener shortener(db_host, db_port);

        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);
 
        thread serverThread([&shortener]() {
            shortener.start(8080);
        });
        
        cout << "Service is running. Press Ctrl+C to stop." << endl;

        while (running) {
            this_thread::sleep_for(chrono::seconds(1));
        }
        
        cout << "Shutting down" << endl;
        
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}
