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
#include "sqlite3.h"
#include "db.h"
#include "sha256.h"
#include "oper.h"
#include "utils.h"

#include <vector>

std::mutex mutex_db;
std::mutex mutex_query;

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
	sqlite3 *database;
	sqlite3_stmt *selectStmt;
	int s;
	std::string retorno;
	Oper oper;
	if (SQLITE_OK != (s = sqlite3_open_v2("file:zeus.db", &database, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI | SQLITE_OPEN_NOMUTEX, NULL)))
	{
	    oper.GlobOPs(Utils::make_string("", "Failure at DB connection"));
	}
	if (SQLITE_OK != (s = sqlite3_prepare_v2(database,sql.c_str(), -1, &selectStmt, NULL)))
    {
    	std::string mensaje = Utils::make_string("", "Failed to prepare insert: ");
    	mensaje.append(sqlite3_errmsg(database));
        oper.GlobOPs(mensaje);
    }
	sqlite3_step(selectStmt);
	if (sqlite3_data_count(selectStmt) > 0)
		retorno = std::string( reinterpret_cast< const char* >(sqlite3_column_text(selectStmt, 0) ) );
	else
		retorno = "";
	sqlite3_finalize(selectStmt);
    sqlite3_close(database);
	return retorno;
}

std::vector<std::vector<std::string> > DB::SQLiteReturnVectorVector (std::string sql) {
	sqlite3 *database;
	sqlite3_stmt *selectStmt;
	int s;
	std::vector<std::vector<std::string> > resultados;
	Oper oper;
	if (SQLITE_OK != (s = sqlite3_open_v2("file:zeus.db", &database, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI | SQLITE_OPEN_NOMUTEX, NULL)))
	{
	    oper.GlobOPs(Utils::make_string("", "Failure at DB connection"));
	}
	if (SQLITE_OK != (sqlite3_prepare_v2(database,sql.c_str(), -1, &selectStmt, NULL)))
    {
    	std::string mensaje = Utils::make_string("", "Failed to prepare insert: ");
    	mensaje.append(sqlite3_errmsg(database));
        oper.GlobOPs(mensaje);
    }
    int cols = sqlite3_column_count(selectStmt);
    while(true)
	{
		if(sqlite3_step (selectStmt) == SQLITE_ROW)
		{
			std::vector<std::string> values;
			for(int col = 0; col < cols; col++)
			{
				values.push_back(std::string( reinterpret_cast< const char* >(sqlite3_column_text(selectStmt, col) ) ));
			}
			resultados.push_back(values);
		}
		else
		{
			break;  
		}
	}
	sqlite3_finalize(selectStmt);
	sqlite3_close(database);
	return resultados;
}

std::vector <std::string> DB::SQLiteReturnVector (std::string sql) {
	sqlite3 *database;
	sqlite3_stmt *selectStmt;
	int s;
	std::vector <std::string> resultados;
	Oper oper;
	if (SQLITE_OK != (s = sqlite3_open_v2("file:zeus.db", &database, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI | SQLITE_OPEN_NOMUTEX, NULL)))
	{
	    oper.GlobOPs(Utils::make_string("", "Failure at DB connection"));
	}
	if (SQLITE_OK != (sqlite3_prepare_v2(database,sql.c_str(), -1, &selectStmt, NULL)))
    {
    	std::string mensaje = Utils::make_string("", "Failed to prepare insert: ");
    	mensaje.append(sqlite3_errmsg(database));
        oper.GlobOPs(mensaje);
    }
	while (sqlite3_step (selectStmt) == SQLITE_ROW) {
		resultados.push_back(std::string( reinterpret_cast< const char* >(sqlite3_column_text(selectStmt, 0) ) ));
	}
	sqlite3_finalize(selectStmt);
    sqlite3_close(database);
	return resultados;
}

int DB::SQLiteReturnInt (std::string sql) {
	sqlite3 *database;
	sqlite3_stmt *selectStmt;
	int s, result = 0;
	Oper oper;
	if (SQLITE_OK != (s = sqlite3_open_v2("file:zeus.db", &database, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI | SQLITE_OPEN_NOMUTEX, NULL)))
	{
	    oper.GlobOPs(Utils::make_string("", "Failure at DB connection"));
	}
	if (SQLITE_OK != (s = sqlite3_prepare_v2(database,sql.c_str(), -1, &selectStmt, NULL)))
    {
    	std::string mensaje = Utils::make_string("", "Failed to prepare insert: ");
    	mensaje.append(sqlite3_errmsg(database));
        oper.GlobOPs(mensaje);
    }
    sqlite3_step(selectStmt);
	if (sqlite3_data_count(selectStmt) > 0)
    	result = sqlite3_column_int (selectStmt, 0);
    	
	sqlite3_finalize(selectStmt);
    sqlite3_close(database);
	return result;
}

bool DB::SQLiteNoReturn (std::string sql) {
	sqlite3 *database;
	sqlite3_stmt *selectStmt;
	int s;
	bool retorno = true;
	Oper oper;
	mutex_query.lock();
	if (SQLITE_OK != (s = sqlite3_open_v2("file:zeus.db", &database, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_URI | SQLITE_OPEN_NOMUTEX, NULL)))
	{
	    oper.GlobOPs(Utils::make_string("", "Failure at DB connection"));
	    retorno = false;
	}
	if (SQLITE_OK != (sqlite3_prepare_v2(database,sql.c_str(), -1, &selectStmt, NULL)))
    {
    	std::string mensaje = Utils::make_string("", "Failed to prepare insert: ");
    	mensaje.append(sqlite3_errmsg(database));
        oper.GlobOPs(mensaje);
        retorno = false;
    }
    if (SQLITE_DONE != (sqlite3_step(selectStmt)))
    {
    	std::string mensaje = Utils::make_string("", "Cannot insert the record.");
    	mensaje.append(sqlite3_errmsg(database));
        oper.GlobOPs(mensaje);
        retorno = false;
    }
	sqlite3_finalize(selectStmt);
    sqlite3_close(database);
    mutex_query.unlock();
    return retorno;
}
