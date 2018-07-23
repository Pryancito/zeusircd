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
<<<<<<< HEAD
		static std::string make_string(const std::string& format, ...);
=======
		static std::string make_string(const std::string nickname, const std::string& fmt, ...);
>>>>>>> 908f304d1ec5e8396109a7795504e63c122e8892
};
