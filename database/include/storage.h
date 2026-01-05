#ifndef STORAGE_H
#define STORAGE_H

#include "database.h"
#include "command_parser.h"
#include <string>

using namespace std;

class Storage {
private:
    Database db;

public:
    Storage(const string& db_file = "/data/database.db");
    string executeCommand(const string& command);
};

#endif
