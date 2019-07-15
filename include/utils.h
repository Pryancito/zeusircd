/* 
 * This file is part of the ZeusiRCd distribution (https://github.com/Pryancito/zeusircd).
 * Copyright (c) 2019 Rodrigo Santidrian AKA Pryan.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

#include <string>
#include <stdlib.h>
#include <stdarg.h>
#include <cstdio>
#include <time.h>
#include <map>

#include "user.h"

typedef std::map<std::string, unsigned int> ForceMap;

class Utils
{
	public:
		static bool Match(const char *first, const char *second);
		static std::string Time(time_t tiempo);
		static bool isnum(const std::string &cadena);
		static std::string make_string(const std::string& nickname, const std::string fmt, ...);
		static std::string GetEmoji(const std::string &ip);
		static std::string GetGeoIP(const std::string &ip);
};

