#ifndef DB_H
#define DB_H

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
};

#endif
