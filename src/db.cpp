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
#include "sqlite.h"
#include "db.h"
#include "sha256.h"
#include "oper.h"
#include "utils.h"
#include "Bayes.h"

#include <vector>

std::mutex mutex_db;

void DB::InitSPAM() {
	bayes->loadspam("spam.txt");
	bayes->loadham("ham.txt");
	StrVec vect;
	std::string sql = "SELECT MASK from SPAM WHERE TARGET LIKE '%P%' COLLATE NOCASE OR TARGET LIKE '%C%' COLLATE NOCASE OR TARGET LIKE '%N%' COLLATE NOCASE;";
	vect = DB::SQLiteReturnVector(sql);
	for (unsigned int i = 0; i < vect.size(); i++) {
		boost::to_lower(vect[i]);
		bayes->learn(1, vect[i].c_str());
	}
	sql = "SELECT MASK from SPAM WHERE TARGET LIKE '%E%' COLLATE NOCASE;";
	vect = DB::SQLiteReturnVector(sql);
	for (unsigned int i = 0; i < vect.size(); i++) {
		boost::to_lower(vect[i]);
		bayes->learn(0, vect[i].c_str());
	}
}

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

int DB::Sync(Servidor *server, const std::string &id) {
	std::vector <std::string> datos;
	std::string sql = "SELECT rowid FROM LAST WHERE ID = '" + id + "' LIMIT 1;";
	std::string rowid = DB::SQLiteReturnString(sql);
	if (id == "0")
		rowid = "0";
	sql = "SELECT TEXTO FROM LAST WHERE rowid > " + rowid + " ORDER BY rowid ASC;";
	datos = DB::SQLiteReturnVector(sql);
	for (unsigned int i = 0; i < datos.size(); i++) {
		server->send(datos[i] + config->EOFServer);
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
	std::string sql = "CREATE TABLE IF NOT EXISTS NICKS (NICKNAME TEXT UNIQUE NOT NULL,PASS TEXT NOT NULL, EMAIL TEXT,URL TEXT, VHOST TEXT, REGISTERED INT , LASTUSED INT );";
    if (DB::SQLiteNoReturn(sql) == false) {
    	std::cout << Utils::make_string("", "Error at create the database %s.", "NICKS") << std::endl;
    	exit(0);
	}
    
    sql = "CREATE TABLE IF NOT EXISTS OPTIONS (NICKNAME TEXT UNIQUE NOT NULL, NOACCESS INT , SHOWMAIL INT, NOMEMO INT, NOOP INT, ONLYREG INT, LANG TEXT );";
     
    if (DB::SQLiteNoReturn(sql) == false) {
    	std::cout << Utils::make_string("", "Error at create the database %s.", "OPTIONS") << std::endl;
    	exit(0);
	}

	sql = "CREATE TABLE IF NOT EXISTS CANALES (NOMBRE TEXT UNIQUE NOT NULL, OWNER TEXT, MODOS TEXT, KEY TEXT, TOPIC TEXT, REGISTERED INT, LASTUSED INT );";
     
    if (DB::SQLiteNoReturn(sql) == false) {
    	std::cout << Utils::make_string("", "Error at create the database %s.", "CANALES") << std::endl;
    	exit(0);
	}

	sql = "CREATE TABLE IF NOT EXISTS ACCESS (CANAL TEXT, ACCESO TEXT , USUARIO TEXT, ADDED TEXT );";
     
    if (DB::SQLiteNoReturn(sql) == false) {
    	std::cout << Utils::make_string("", "Error at create the database %s.", "ACCESS") << std::endl;
    	exit(0);
	}

	sql = "CREATE TABLE IF NOT EXISTS AKICK (CANAL TEXT, MASCARA TEXT , MOTIVO TEXT, ADDED TEXT );";
     
    if (DB::SQLiteNoReturn(sql) == false) {
    	std::cout << Utils::make_string("", "Error at create the database %s.", "AKICK") << std::endl;
    	exit(0);
	}
	
    sql = "CREATE TABLE IF NOT EXISTS LAST (rowid INTEGER PRIMARY KEY ASC, ID TEXT UNIQUE NOT NULL, TEXTO  TEXT    NOT NULL, FECHA INT );";
    if (DB::SQLiteNoReturn(sql) == false) {
    	std::cout << Utils::make_string("", "Error at create the database %s.", "LAST") << std::endl;
    	exit(0);
	}
	
	sql = "CREATE TABLE IF NOT EXISTS GLINE (IP TEXT UNIQUE NOT NULL, MOTIVO  TEXT, NICK TEXT );";
    if (DB::SQLiteNoReturn(sql) == false) {
    	std::cout << Utils::make_string("", "Error at create the database %s.", "GLINE") << std::endl;
    	exit(0);
	}
	
	sql = "CREATE TABLE IF NOT EXISTS PATHS (OWNER TEXT, PATH TEXT UNIQUE NOT NULL);";
    if (DB::SQLiteNoReturn(sql) == false) {
    	std::cout << Utils::make_string("", "Error at create the database %s.", "PATHS") << std::endl;
    	exit(0);
	}
	
	sql = "CREATE TABLE IF NOT EXISTS REQUEST (OWNER TEXT UNIQUE NOT NULL, PATH TEXT, TIME INT );";
    if (DB::SQLiteNoReturn(sql) == false) {
    	std::cout << Utils::make_string("", "Error at create the database %s.", "REQUEST") << std::endl;
    	exit(0);
	}
	
	sql = "CREATE TABLE IF NOT EXISTS CMODES (CANAL TEXT UNIQUE NOT NULL, FLOOD INT, ONLYREG INT, AUTOVOICE INT, MODERATED INT, ONLYSECURE INT, NONICKCHANGE INT, ONLYWEB INT );";
    if (DB::SQLiteNoReturn(sql) == false) {
    	std::cout << Utils::make_string("", "Error at create the database %s.", "CMODES") << std::endl;
    	exit(0);
	}
	
	sql = "CREATE TABLE IF NOT EXISTS SPAM (MASK TEXT UNIQUE NOT NULL, WHO TEXT, MOTIVO TEXT, TARGET TEXT );";
    if (DB::SQLiteNoReturn(sql) == false) {
    	std::cout << Utils::make_string("", "Error at create the database %s.", "SPAM") << std::endl;
    	exit(0);
	}
	
	sql = "CREATE TABLE IF NOT EXISTS OPERS (NICK TEXT UNIQUE NOT NULL, OPERBY TEXT, TIEMPO INT );";
    if (DB::SQLiteNoReturn(sql) == false) {
    	std::cout << Utils::make_string("", "Error at create the database %s.", "OPERS") << std::endl;
    	exit(0);
	}
	
	sql = "CREATE TABLE IF NOT EXISTS EXCEPTIONS (IP TEXT, OPTION TEXT, VALUE TEXT, ADDED TEXT, DATE INT );";
    if (DB::SQLiteNoReturn(sql) == false) {
    	std::cout << Utils::make_string("", "Error at create the database %s.", "EXCEPTIONS") << std::endl;
    	exit(0);
	}
	
	return;
}

std::string DB::SQLiteReturnString (std::string sql) {
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

std::vector<std::vector<std::string> > DB::SQLiteReturnVectorVector (std::string sql) {
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

std::vector <std::string> DB::SQLiteReturnVector (std::string sql) {
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

int DB::SQLiteReturnInt (std::string sql) {
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
bool DB::SQLiteNoReturn (std::string sql) {
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
