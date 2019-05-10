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
#include <vector>
#include "session.h"

class DB
{
	public:
		static void AlmacenaDB(std::string cadena);
		static std::string GetLastRecord ();
		static int LastInsert();
		static void IniciarDB();
		static std::string SQLiteReturnString (std::string sql);
		static int SQLiteReturnInt (std::string sql);
		static bool SQLiteNoReturn (std::string sql);
		static int Sync(Servidor *server, const std::string &id);
		static std::vector <std::string> SQLiteReturnVector (std::string sql);
		static std::vector<std::vector<std::string> > SQLiteReturnVectorVector (std::string sql);
		static std::string GenerateID();
		static bool EscapeChar(std::string cadena);
		static void InitSPAM();
};
