#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

class Utils
{
	public:
		static bool Match(const char *first, const char *second);
		static std::string Time(time_t tiempo);
		static bool isnum(std::string cadena);
		static std::string make_string(const std::string& nickname, const std::string& fmt, ...);
		static std::string GetEmoji(const std::string &ip);
};

#endif
