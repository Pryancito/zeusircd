#pragma once

#include <string>
#include <vector>
#include "session.h"

class DB
{
	public:
		static void AlmacenaDB(std::string cadena);
		static void BorraDB(std::string id);
		static std::string GetLastRecord ();
		static void IniciarDB();
		static std::string SQLiteReturnString (std::string sql);
		static int SQLiteReturnInt (std::string sql);
		static bool SQLiteNoReturn (std::string sql);
		static int Sync(Servidor *server, std::string id);
		static std::vector <std::string> SQLiteReturnVector (std::string sql);
		static std::string GenerateID();
		static bool EscapeChar(std::string cadena);
};
