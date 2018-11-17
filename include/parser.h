#pragma once

#define GC_THREADS
#define GC_ALWAYS_MULTITHREADED
#include <gc_cpp.h>
#include <gc.h>

#include "user.h"
#include <string>

class Parser : public gc_cleanup {
    
public:

	static void parse(std::string& message, User* user);
	Parser() = delete;
	static bool checknick (const std::string &nick);
	static bool checkchan (const std::string &chan);
	static void log (const std::string &message);
};

