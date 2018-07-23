#pragma once

#include <string>

class Utils
{
	public:
		static bool Match(const char *first, const char *second);
		static std::string Time(time_t tiempo);
		static bool isnumber(std::string cadena);
		static std::string make_string(const std::string& fmt, ...);
};
