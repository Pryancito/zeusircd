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
#include "db.h"
#include "oper.h"
#include "Utils.h"
#include "Config.h"
#include "sha256.h"
#include "Server.h"

#include <vector>
#include <string>
#include <iterator>
#include <algorithm>

std::mutex mutex_db;

bool DB::EscapeChar(std::string cadena) {
	for (unsigned int i = 0; i < cadena.length(); i++) {
        if (strchr("\"'\r\n\t",cadena[i]))
        {
            return true;
        }
    }
    return false;
}

void DB::AlmacenaDB(std::string cadena) {
	mutex_db.lock();
	std::string id = cadena.substr(3, 32);
	std::string sql = "SELECT MAX(rowid) FROM LAST;";
	int rowid = DB::SQLiteReturnInt(sql);
	sql = "INSERT INTO LAST VALUES (" + std::to_string(rowid+1) + ", '" + id + "', \"" + cadena + "\", " + std::to_string(time(0)) + ");";
	DB::SQLiteNoReturn(sql);
	mutex_db.unlock();
	return;
}

int DB::Sync(std::string server, const std::string &id) {
	std::vector <std::string> datos;
	std::string sql = "SELECT rowid FROM LAST WHERE ID = '" + id + "' LIMIT 1;";
	std::string rowid = DB::SQLiteReturnString(sql);
	if (id == "0")
		rowid = "0";
	sql = "SELECT TEXTO FROM LAST WHERE rowid > " + rowid + " ORDER BY rowid ASC;";
	datos = DB::SQLiteReturnVector(sql);
	for (unsigned int i = 0; i < datos.size(); i++) {
		//server->Send(datos[i]);
	}
	return datos.size();
}

std::string DB::GetLastRecord () {
	std::string sql = "SELECT ID FROM LAST ORDER BY rowid DESC LIMIT 1;";
	return DB::SQLiteReturnString(sql);
}

std::string DB::GenerateID() {
	return sha256(std::to_string(rand()%999999999999*rand()%999999*time(0))).substr(0, 32);
}

void DB::IniciarDB () {
	std::string sql;
	if (config->Getvalue("dbtype") == "mysql")
		sql = "CREATE TABLE IF NOT EXISTS NICKS (NICKNAME VARCHAR(255) UNIQUE NOT NULL, PASS TEXT NOT NULL, EMAIL TEXT,URL TEXT, VHOST TEXT, REGISTERED INT , LASTUSED INT );";
	else
		sql = "CREATE TABLE IF NOT EXISTS NICKS (NICKNAME TEXT UNIQUE NOT NULL COLLATE NOCASE, PASS TEXT NOT NULL, EMAIL TEXT,URL TEXT, VHOST TEXT, REGISTERED INT , LASTUSED INT );";
    if (DB::SQLiteNoReturn(sql) == false) {
    	std::cout << Utils::make_string("", "Error at create the database %s.", "NICKS") << std::endl;
	}
   	if (config->Getvalue("dbtype") == "mysql")
		sql = "CREATE TABLE IF NOT EXISTS OPTIONS (NICKNAME VARCHAR(255) UNIQUE NOT NULL, NOACCESS INT DEFAULT 0, SHOWMAIL INT DEFAULT 0, NOMEMO INT DEFAULT 0, NOOP INT DEFAULT 0, ONLYREG INT DEFAULT 0, LANG TEXT, NOCOLOR INT DEFAULT 0 );";
   	else
		sql = "CREATE TABLE IF NOT EXISTS OPTIONS (NICKNAME TEXT UNIQUE NOT NULL COLLATE NOCASE, NOACCESS INT , SHOWMAIL INT, NOMEMO INT, NOOP INT, ONLYREG INT, LANG TEXT, NOCOLOR INT );";
     
    if (DB::SQLiteNoReturn(sql) == false) {
    	std::cout << Utils::make_string("", "Error at create the database %s.", "OPTIONS") << std::endl;
	}
	if (config->Getvalue("dbtype") == "mysql")
		sql = "CREATE TABLE IF NOT EXISTS CANALES (NOMBRE VARCHAR(255) UNIQUE NOT NULL, OWNER TEXT, MODOS TEXT, CLAVE TEXT, TOPIC TEXT, REGISTERED INT, LASTUSED INT );";
    else
		sql = "CREATE TABLE IF NOT EXISTS CANALES (NOMBRE TEXT UNIQUE NOT NULL COLLATE NOCASE, OWNER TEXT COLLATE NOCASE, MODOS TEXT, CLAVE TEXT, TOPIC TEXT, REGISTERED INT, LASTUSED INT );";
    if (DB::SQLiteNoReturn(sql) == false) {
    	std::cout << Utils::make_string("", "Error at create the database %s.", "CANALES") << std::endl;
	}
	if (config->Getvalue("dbtype") == "mysql")
		sql = "CREATE TABLE IF NOT EXISTS ACCESS (CANAL TEXT, ACCESO TEXT , USUARIO TEXT, ADDED TEXT );";
    else
		sql = "CREATE TABLE IF NOT EXISTS ACCESS (CANAL TEXT COLLATE NOCASE, ACCESO TEXT COLLATE NOCASE, USUARIO TEXT COLLATE NOCASE, ADDED TEXT COLLATE NOCASE );";
    if (DB::SQLiteNoReturn(sql) == false) {
    	std::cout << Utils::make_string("", "Error at create the database %s.", "ACCESS") << std::endl;
	}
	if (config->Getvalue("dbtype") == "mysql")
		sql = "CREATE TABLE IF NOT EXISTS AKICK (CANAL TEXT, MASCARA TEXT , MOTIVO TEXT, ADDED TEXT );";
	else
		sql = "CREATE TABLE IF NOT EXISTS AKICK (CANAL TEXT COLLATE NOCASE, MASCARA TEXT , MOTIVO TEXT, ADDED TEXT COLLATE NOCASE );";
     
    if (DB::SQLiteNoReturn(sql) == false) {
    	std::cout << Utils::make_string("", "Error at create the database %s.", "AKICK") << std::endl;
	}
	if (config->Getvalue("dbtype") == "mysql")
		sql = "CREATE TABLE IF NOT EXISTS LAST (rowid INTEGER PRIMARY KEY, ID VARCHAR(255) UNIQUE NOT NULL, TEXTO  TEXT    NOT NULL, FECHA INT );";
	else
		sql = "CREATE TABLE IF NOT EXISTS LAST (rowid INTEGER PRIMARY KEY ASC, ID TEXT UNIQUE NOT NULL, TEXTO  TEXT    NOT NULL, FECHA INT );";
    if (DB::SQLiteNoReturn(sql) == false) {
    	std::cout << Utils::make_string("", "Error at create the database %s.", "LAST") << std::endl;
	}
	
	if (config->Getvalue("dbtype") == "mysql")
		sql = "CREATE TABLE IF NOT EXISTS GLINE (IP VARCHAR(255) UNIQUE NOT NULL, MOTIVO  TEXT, NICK TEXT );";
	else
		sql = "CREATE TABLE IF NOT EXISTS GLINE (IP TEXT UNIQUE NOT NULL, MOTIVO  TEXT, NICK TEXT COLLATE NOCASE );";
    if (DB::SQLiteNoReturn(sql) == false) {
    	std::cout << Utils::make_string("", "Error at create the database %s.", "GLINE") << std::endl;
	}
	
	if (config->Getvalue("dbtype") == "mysql")
		sql = "CREATE TABLE IF NOT EXISTS PATHS (OWNER TEXT, PATH VARCHAR(255) UNIQUE NOT NULL );";
	else
		sql = "CREATE TABLE IF NOT EXISTS PATHS (OWNER TEXT COLLATE NOCASE, PATH TEXT UNIQUE NOT NULL COLLATE NOCASE );";
    if (DB::SQLiteNoReturn(sql) == false) {
    	std::cout << Utils::make_string("", "Error at create the database %s.", "PATHS") << std::endl;
	}
	
	if (config->Getvalue("dbtype") == "mysql")
		sql = "CREATE TABLE IF NOT EXISTS REQUEST (OWNER VARCHAR(255) UNIQUE NOT NULL, PATH TEXT, TIME INT );";
	else
		sql = "CREATE TABLE IF NOT EXISTS REQUEST (OWNER TEXT UNIQUE NOT NULL COLLATE NOCASE, PATH TEXT COLLATE NOCASE, TIME INT );";
    if (DB::SQLiteNoReturn(sql) == false) {
    	std::cout << Utils::make_string("", "Error at create the database %s.", "REQUEST") << std::endl;
	}
	
	if (config->Getvalue("dbtype") == "mysql")
		sql = "CREATE TABLE IF NOT EXISTS CMODES (CANAL VARCHAR(255) UNIQUE NOT NULL, FLOOD INT DEFAULT 0, ONLYREG INT DEFAULT 0, AUTOVOICE INT DEFAULT 0, MODERATED INT DEFAULT 0, ONLYSECURE INT DEFAULT 0, NONICKCHANGE INT DEFAULT 0, ONLYWEB INT DEFAULT 0, COUNTRY TEXT, ONLYACCESS INT DEFAULT 0 );";
	else
		sql = "CREATE TABLE IF NOT EXISTS CMODES (CANAL TEXT UNIQUE NOT NULL COLLATE NOCASE, FLOOD INT, ONLYREG INT, AUTOVOICE INT, MODERATED INT, ONLYSECURE INT, NONICKCHANGE INT, ONLYWEB INT, COUNTRY TEXT, ONLYACCESS INT );";
    if (DB::SQLiteNoReturn(sql) == false) {
    	std::cout << Utils::make_string("", "Error at create the database %s.", "CMODES") << std::endl;
	}
	if (config->Getvalue("dbtype") == "mysql")
		sql = "CREATE TABLE IF NOT EXISTS SPAM (MASK VARCHAR(255) UNIQUE NOT NULL, WHO TEXT, MOTIVO TEXT, TARGET TEXT );";
	else
		sql = "CREATE TABLE IF NOT EXISTS SPAM (MASK TEXT UNIQUE NOT NULL COLLATE NOCASE, WHO TEXT COLLATE NOCASE, MOTIVO TEXT, TARGET TEXT COLLATE NOCASE );";
    if (DB::SQLiteNoReturn(sql) == false) {
    	std::cout << Utils::make_string("", "Error at create the database %s.", "SPAM") << std::endl;
	}
	
	if (config->Getvalue("dbtype") == "mysql")
		sql = "CREATE TABLE IF NOT EXISTS OPERS (NICK VARCHAR(255) UNIQUE NOT NULL, OPERBY TEXT, TIEMPO INT );";
	else
		sql = "CREATE TABLE IF NOT EXISTS OPERS (NICK TEXT UNIQUE NOT NULL COLLATE NOCASE, OPERBY TEXT COLLATE NOCASE, TIEMPO INT );";
    if (DB::SQLiteNoReturn(sql) == false) {
    	std::cout << Utils::make_string("", "Error at create the database %s.", "OPERS") << std::endl;
	}
	if (config->Getvalue("dbtype") == "mysql")
		sql = "CREATE TABLE IF NOT EXISTS EXCEPTIONS (IP TEXT, OPTION TEXT, VALUE TEXT, ADDED TEXT, DATE INT );";
	else
		sql = "CREATE TABLE IF NOT EXISTS EXCEPTIONS (IP TEXT, OPTION TEXT, VALUE TEXT, ADDED TEXT COLLATE NOCASE, DATE INT );";
    if (DB::SQLiteNoReturn(sql) == false) {
    	std::cout << Utils::make_string("", "Error at create the database %s.", "EXCEPTIONS") << std::endl;
	}
	
	return;
}

std::string DB::SQLiteReturnString (std::string sql) {
	if (config->Getvalue("dbtype") == "mysql") {
		try {
			mysql::connect_options options;
			options.server = config->Getvalue("dbhost");
			options.username = config->Getvalue("dbuser");
			options.password = config->Getvalue("dbpass");
			options.dbname = config->Getvalue("dbname");
			options.timeout = 30;
			options.autoreconnect = true;
			options.init_command = "";
			options.charset = "";
			options.port = (unsigned int ) stoi(config->Getvalue("dbport"));

			mysql::connection my{ options };
			if (!my)
				return "";
			return my.query(sql.c_str()).get_value<std::string>();
		} catch (...) {
			return "";
		}
	} else {
		try {
			sqlite::sqlite db("zeus.db", true);
			sqlite::statement_ptr s = db.get_statement();
			s->set_sql(sql.c_str());
			s->prepare();
			s->step();
			return s->get_text(0);
		} catch (...) {
			return "";
		}
	}
}

std::vector<std::vector<std::string> > DB::SQLiteReturnVectorVector (std::string sql) {
	if (config->Getvalue("dbtype") == "mysql") {
		try {
			std::vector<std::vector<std::string> > resultados;
			mysql::connect_options options;
			options.server = config->Getvalue("dbhost");
			options.username = config->Getvalue("dbuser");
			options.password = config->Getvalue("dbpass");
			options.dbname = config->Getvalue("dbname");
			options.timeout = 30;
			options.autoreconnect = true;
			options.init_command = "";
			options.charset = "";
			options.port = (unsigned int ) stoi(config->Getvalue("dbport"));

			mysql::connection my{ options };
			if (!my)
				return resultados;
			auto res = my.query(sql.c_str());
			res.fetch();
			resultados = res.fetch_vector();
			return resultados;
		} catch (...) {
			std::vector<std::vector<std::string> > resultados;
			return resultados;
		}
	} else {
		try {
			std::vector<std::vector<std::string> > resultados;
			sqlite::sqlite db("zeus.db", true);
			sqlite::statement_ptr s = db.get_statement();
			s->set_sql(sql.c_str());
			s->prepare();
			while(s->step())
			{
				std::vector<std::string> values;
				for(int col = 0; col < s->get_columns(); col++)
				{
					values.push_back(std::string(s->get_text(col)));
				}
				resultados.push_back(values);
			}
			return resultados;
		} catch (...) {
			std::vector<std::vector<std::string> > resultados;
			return resultados;
		}
	}
}

std::vector <std::string> DB::SQLiteReturnVector (std::string sql) {
	if (config->Getvalue("dbtype") == "mysql") {
		try {
			std::vector <std::string> resultados;
			mysql::connect_options options;
			options.server = config->Getvalue("dbhost");
			options.username = config->Getvalue("dbuser");
			options.password = config->Getvalue("dbpass");
			options.dbname = config->Getvalue("dbname");
			options.timeout = 30;
			options.autoreconnect = true;
			options.init_command = "";
			options.charset = "";
			options.port = (unsigned int ) stoi(config->Getvalue("dbport"));

			mysql::connection my{ options };
			if (!my)
				return resultados;
			auto res = my.query(sql.c_str());
			std::string dato;
			while (!res.eof()) {
				res.fetch(dato);
				resultados.push_back(dato);
				res.next();
			}
		} catch (...) {
			std::vector <std::string> resultados;
			return resultados;
		}
	} else {
		try {
			std::vector <std::string> resultados;
			sqlite::sqlite db("zeus.db", true);
			sqlite::statement_ptr s = db.get_statement();
			s->set_sql(sql.c_str());
			s->prepare();
			while(s->step())
			{
				resultados.push_back(std::string(s->get_text(0)));
			}
			return resultados;
		} catch (...) {
			std::vector <std::string> resultados;
			return resultados;
		}
	}
	std::vector <std::string> resultados;
	return resultados;
}

int DB::SQLiteReturnInt (std::string sql) {
	if (config->Getvalue("dbtype") == "mysql") {
		try {
			mysql::connect_options options;
			options.server = config->Getvalue("dbhost");
			options.username = config->Getvalue("dbuser");
			options.password = config->Getvalue("dbpass");
			options.dbname = config->Getvalue("dbname");
			options.timeout = 30;
			options.autoreconnect = true;
			options.init_command = "";
			options.charset = "";
			options.port = (unsigned int ) stoi(config->Getvalue("dbport"));

			mysql::connection my{ options };
			if (!my)
				return 0;
			return my.query(sql.c_str()).get_value<int>();
		} catch (...) {
			return 0;
		}
	} else {
		try {
			sqlite::sqlite db("zeus.db", true);
			sqlite::statement_ptr s = db.get_statement();
			s->set_sql(sql.c_str());
			s->prepare();
			s->step();
			return s->get_int(0);
		} catch (...) {
			return 0;
		}
	}
}
bool DB::SQLiteNoReturn (std::string sql) {
	if (config->Getvalue("dbtype") == "mysql") {
		try {
			mysql::connect_options options;
			options.server = config->Getvalue("dbhost");
			options.username = config->Getvalue("dbuser");
			options.password = config->Getvalue("dbpass");
			options.dbname = config->Getvalue("dbname");
			options.timeout = 30;
			options.autoreconnect = true;
			options.init_command = "";
			options.charset = "";
			options.port = (unsigned int ) stoi(config->Getvalue("dbport"));

			mysql::connection my{ options };

			if (!my)
				return false;
			my.exec(sql.c_str());
				return true;
		} catch (...) {
			return false;
		}
	} else {
		try {
			sqlite::sqlite db("zeus.db", false);
			sqlite::statement_ptr s = db.get_statement();
			s->set_sql(sql.c_str());
			s->exec();
			s->reset();
			return true;
		} catch (...) {
			return false;
		}
	}
}
