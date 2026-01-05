#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include "database.h"
#include <string>

std::string processCommand(Database& db, const std::string& command);

#endif
