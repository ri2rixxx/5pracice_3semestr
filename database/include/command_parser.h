#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include <string>
#include <vector>

using namespace std;

class CommandParser {
public:
    static vector<string> parse(const string& command);
};

#endif
