#pragma once

#include <string>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

class Utils
{
	public:
		static bool Match(const char *first, const char *second);
		static std::string Time(time_t tiempo);
		static bool isnumber(std::string cadena);
		static std::string make_string(const std::string& format, ...);
};
