#include "command_processor.h"
#include <sstream>
#include <algorithm>
#include <iostream>
#include <vector>

using namespace std;

string processCommand(Database& db, const string& command) {
    istringstream iss(command);
    vector<string> tokens;
    string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    if (tokens.empty()) {
        return "-ERR no command provided";
    }
    
    string cmd = tokens[0];
    transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    
    try {
        if (cmd == "PING") {
            return "+PONG";
        }
        else if (cmd == "SET") {
            if (tokens.size() < 3) {
                return "-ERR wrong number of arguments for 'SET'";
            }
            db.set(tokens[1], tokens[2]);
            return "+OK";
        }
        else if (cmd == "GET") {
            if (tokens.size() < 2) {
                return "-ERR wrong number of arguments for 'GET'";
            }
            string value = db.get(tokens[1]);
            if (value.empty()) {
                return "(nil)";
            }
            return value;
        }
        else if (cmd == "DEL") {
            if (tokens.size() < 2) {
                return "-ERR wrong number of arguments for 'DEL'";
            }
            if (db.del(tokens[1])) {
                return "+OK";
            }
            return "-ERR key not found";
        }
        else if (cmd == "INCR") {
            if (tokens.size() < 2) {
                return "-ERR wrong number of arguments for 'INCR'";
            }
            int result = db.incr(tokens[1]);
            return to_string(result);
        }
        else if (cmd == "HSET") {
            if (tokens.size() < 4) {
                return "-ERR wrong number of arguments for 'HSET'";
            }
            return db.hset(tokens[1], tokens[2], tokens[3]);
        }
        else if (cmd == "HGET") {
            if (tokens.size() < 3) {
                return "-ERR wrong number of arguments for 'HGET'";
            }
            return db.hget(tokens[1], tokens[2]);
        }
        else if (cmd == "HGETALL") {
            if (tokens.size() < 2) {
                return "-ERR wrong number of arguments for 'HGETALL'";
            }
            return db.hgetall(tokens[1]);
        }
        else if (cmd == "SADD") {
            if (tokens.size() < 3) {
                return "-ERR wrong number of arguments for 'SADD'";
            }
            return db.sadd(tokens[1], tokens[2]);
        }
        else if (cmd == "SMEMBERS") {
            if (tokens.size() < 2) {
                return "-ERR wrong number of arguments for 'SMEMBERS'";
            }
            return db.smembers(tokens[1]);
        }
        else if (cmd == "QPUSH") {
            if (tokens.size() < 3) {
                return "-ERR wrong number of arguments for 'QPUSH'";
            }
            return db.qpush(tokens[1], tokens[2]);
        }
        else if (cmd == "QPOP") {
            if (tokens.size() < 2) {
                return "-ERR wrong number of arguments for 'QPOP'";
            }
            return db.qpop(tokens[1]);
        }
        else if (cmd == "KEYS") {
            return db.keys();
        }
        else if (cmd == "SAVE") {
            db.save();
            return "+OK";
        }
        else {
            return "-ERR unknown command '" + cmd + "'";
        }
    } catch (const exception& e) {
        return "-ERR " + string(e.what());
    }
}
