#pragma once

#include "user.h"
#include <string>

class Parser {
    
public:

	static void parse(std::string& message, User* user);
	Parser() = delete;
	static bool checknick (const std::string &nick);
	static bool checkchan (const std::string &chan);
	static void log (const std::string &message);
};

