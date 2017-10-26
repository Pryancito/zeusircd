#include "include.h"
#include "sha256.h"

using namespace std;

void DB::AlmacenaDB(string cadena) {
	string id = cadena.substr(3, 16);
	string sql = "INSERT INTO LAST VALUES ('" + id + "', \"" + cadena + "\", " + boost::to_string(time(0)) + ");";
	DB::SQLiteNoReturn(sql);
	return;
}

void DB::BorraDB(string id) {
	string sql = "DELETE FROM LAST WHERE ID = '" + id + "';";
	DB::SQLiteNoReturn(sql);
	return;
}

int DB::Sync(Socket *s, string id) {
	vector <string> datos;
	string sql = "SELECT FECHA FROM LAST WHERE ID = '" + id + "' LIMIT 1;";
	string fecha = DB::SQLiteReturnString(sql);
	if (fecha.length() == 0 || id == "0")
		fecha = "0";
	sql = "SELECT TEXTO FROM LAST WHERE FECHA > " + fecha + " ORDER BY FECHA ASC;";
	datos = DB::SQLiteReturnVector(sql);
	for (unsigned int i = 0; i < datos.size(); i++) {
		s->Write(datos[i] + '\n');
	}
	return datos.size();
}

string DB::GetLastRecord () {
	string sql = "SELECT ID FROM LAST ORDER BY FECHA DESC LIMIT 1;";
	string ret = DB::SQLiteReturnString(sql);
	return ret;
}

string DB::GenerateID() {
	return sha256(boost::to_string(rand()%rand())).substr(0, 16);
}

void DB::IniciarDB () {
	string sql = "CREATE TABLE IF NOT EXISTS NICKS (NICKNAME TEXT UNIQUE NOT NULL,PASS TEXT NOT NULL, EMAIL TEXT,URL TEXT, VHOST TEXT, REGISTERED INT , LASTUSED INT );";
    if (DB::SQLiteNoReturn(sql) == false) {
    	cout << "Error al crear las bases de datos NICKS." << endl;
    	exit(0);
	}
    
    sql = "CREATE TABLE IF NOT EXISTS OPTIONS (NICKNAME TEXT UNIQUE NOT NULL, NOACCESS INT , SHOWMAIL INT, NOMEMO INT, NOOP INT, ONLYREG INT );";
     
    if (DB::SQLiteNoReturn(sql) == false) {
    	cout << "Error al crear las bases de datos OPTIONS." << endl;
    	exit(0);
	}

	sql = "CREATE TABLE IF NOT EXISTS CANALES (NOMBRE TEXT UNIQUE NOT NULL, OWNER TEXT, MODOS TEXT, KEY TEXT, TOPIC TEXT, REGISTERED INT, LASTUSED INT );";
     
    if (DB::SQLiteNoReturn(sql) == false) {
    	cout << "Error al crear las bases de datos CANALES." << endl;
    	exit(0);
	}

	sql = "CREATE TABLE IF NOT EXISTS ACCESS (CANAL TEXT, ACCESO TEXT , USUARIO TEXT, ADDED TEXT );";
     
    if (DB::SQLiteNoReturn(sql) == false) {
    	cout << "Error al crear las bases de datos ACCESS." << endl;
    	exit(0);
	}

	sql = "CREATE TABLE IF NOT EXISTS AKICK (CANAL TEXT, MASCARA TEXT , MOTIVO TEXT, ADDED TEXT );";
     
    if (DB::SQLiteNoReturn(sql) == false) {
    	cout << "Error al crear las bases de datos AKICK." << endl;
    	exit(0);
	}
	
    sql = "CREATE TABLE IF NOT EXISTS LAST (ID TEXT UNIQUE NOT NULL, TEXTO  TEXT    NOT NULL, FECHA INT );";
    if (DB::SQLiteNoReturn(sql) == false) {
    	cout << "Error al crear las bases de datos LAST." << endl;
    	exit(0);
	}
	
	sql = "CREATE TABLE IF NOT EXISTS GLINE (IP UNIQUE NOT NULL, MOTIVO  TEXT, NICK TEXT );";
    if (DB::SQLiteNoReturn(sql) == false) {
    	cout << "Error al crear las bases de datos LAST." << endl;
    	exit(0);
	}
	
	return;
}

string DB::SQLiteReturnString (string sql) {
	sqlite3 *database;
	sqlite3_stmt *selectStmt;
	int s;
	string retorno;
	
	if (SQLITE_OK != (s = sqlite3_open_v2("file:zeus.db", &database, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI, NULL)))
	{
	    Oper::GlobOPs("Fallo al conectar a la BDD");
	}
	if (SQLITE_OK != (s = sqlite3_prepare_v2(database,sql.c_str(), -1, &selectStmt, NULL)))
    {
    	string mensaje = "Failed to prepare insert: ";
    	mensaje.append(sqlite3_errmsg(database));
        Oper::GlobOPs(mensaje);
    }
	sqlite3_step(selectStmt);
	if (sqlite3_data_count(selectStmt) > 0)
		retorno = string( reinterpret_cast< const char* >(sqlite3_column_text(selectStmt, 0) ) );
	else
		retorno = "";
	if (NULL != selectStmt) sqlite3_finalize(selectStmt);
    if (NULL != database) sqlite3_close(database);
	return retorno;
}

vector <string> DB::SQLiteReturnVector (string sql) {
	sqlite3 *database;
	sqlite3_stmt *selectStmt;
	int s;
	vector <string> resultados;
	if (SQLITE_OK != (s = sqlite3_open_v2("file:zeus.db", &database, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI, NULL)))
	{
	    Oper::GlobOPs("Fallo al conectar a la BDD");
	}
	if (SQLITE_OK != (sqlite3_prepare_v2(database,sql.c_str(), -1, &selectStmt, NULL)))
    {
    	string mensaje = "Failed to prepare insert: ";
    	mensaje.append(sqlite3_errmsg(database));
        Oper::GlobOPs(mensaje);
    }
	while (sqlite3_step (selectStmt) == SQLITE_ROW) {
		resultados.push_back(string( reinterpret_cast< const char* >(sqlite3_column_text(selectStmt, 0) ) ));
	}
	if (NULL != selectStmt) sqlite3_finalize(selectStmt);
    if (NULL != database) sqlite3_close(database);
	return resultados;
}

int DB::SQLiteReturnInt (string sql) {
	sqlite3 *database;
	sqlite3_stmt *selectStmt;
	int s, result = 0;
	
	if (SQLITE_OK != (s = sqlite3_open_v2("file:zeus.db", &database, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI, NULL)))
	{
	    Oper::GlobOPs("Fallo al conectar a la BDD");
	}
	if (SQLITE_OK != (s = sqlite3_prepare_v2(database,sql.c_str(), -1, &selectStmt, NULL)))
    {
    	string mensaje = "Failed to prepare insert: ";
    	mensaje.append(sqlite3_errmsg(database));
        Oper::GlobOPs(mensaje);
    }
    sqlite3_step(selectStmt);
	if (sqlite3_data_count(selectStmt) > 0)
    	result = sqlite3_column_int (selectStmt, 0);
    	
	if (NULL != selectStmt) sqlite3_finalize(selectStmt);
    if (NULL != database) sqlite3_close(database);
	return result;
}

bool DB::SQLiteNoReturn (string sql) {
	sqlite3 *database;
	sqlite3_stmt *selectStmt;
	int s;
	bool retorno = true;
	if (SQLITE_OK != (s = sqlite3_open_v2("file:zeus.db", &database, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_URI, NULL)))
	{
	    Oper::GlobOPs("Fallo al conectar a la BDD");
	    retorno = false;
	}
	if (SQLITE_OK != (sqlite3_prepare_v2(database,sql.c_str(), -1, &selectStmt, NULL)))
    {
    	string mensaje = "Failed to prepare insert: ";
    	mensaje.append(sqlite3_errmsg(database));
        Oper::GlobOPs(mensaje);
        retorno = false;
    }
    if (SQLITE_DONE != (sqlite3_step(selectStmt)))
    {
    	string mensaje = "No se pudo insertar el registro: ";
    	mensaje.append(sqlite3_errmsg(database));
        Oper::GlobOPs(mensaje);
        retorno = false;
    }
	if (NULL != selectStmt) sqlite3_finalize(selectStmt);
    if (NULL != database) sqlite3_close(database);
    return retorno;
}
