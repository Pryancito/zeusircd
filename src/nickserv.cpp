#include "include.h"
#include "sha256.h"
#include <regex>

using namespace std;

NickServ *nickserv = new NickServ();

std::vector<std::string> split(const std::string &str, int delimiter(int) = ::isspace);

void NickServ::ProcesaMensaje(TCPStream *stream, string mensaje) {
	if (mensaje.length() == 0 || mensaje == "\r\n" || mensaje == "\r" || mensaje == "\n" || mensaje == "||")
		return;
	vector<string> x = split(mensaje);
	string cmd = x[0];
	mayuscula(cmd);
	int sID = datos->BuscarIDStream(stream);
	
	if (cmd == "REGISTER") {
		if (x.size() < 2) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :Necesito mas datos." + "\r\n");
			return;
		} else if (sID < 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return;
		} else if (nickserv->IsRegistered(nick->GetNick(sID)) == 1) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :El nick ya esta registrado." + "\r\n");
			return;
		} else if (server->HUBExiste() == 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :El HUB no existe, las BDs estan en modo de solo lectura." + "\r\n");
			return;
		} else {
			string sql = "INSERT INTO NICKS VALUES ( '" + nick->GetNick(sID) + "', '" + sha256(x[1]) + "', '', '', '',  " + to_string(time(0)) + ", " + to_string(time(0)) + " );";
			if (db->SQLiteNoReturn(sql) == false) {
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :El nick " + nick->GetNick(sID) + " no ha sido registrado.\r\n");
				return;
			}
			sql = "DB " + db->GenerateID() + " " + sql;
			db->AlmacenaDB(sql);
			server->SendToAllServers(sql);
			sql = "INSERT INTO OPTIONS VALUES ( '" + nick->GetNick(sID) + "', 0, 0, 0, 0, 0);";
			db->SQLiteNoReturn(sql);
			sql = "DB " + db->GenerateID() + " " + sql;
			db->AlmacenaDB(sql);
			server->SendToAllServers(sql);
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :El nick " + nick->GetNick(sID) + " ha sido registrado.\r\n");
			if (datos->nicks[sID]->tiene_r == false) {
				sock->Write(stream, ":" + config->Getvalue("serverName") + " MODE " + nick->GetNick(sID) + " +r\r\n");
				datos->nicks[sID]->tiene_r = true;
			}
			return;
		}
	} else if (cmd == "DROP") {
		if (x.size() < 2) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :Necesito mas datos." + "\r\n");
			return;
		} else if (sID < 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return;
		} else if (nickserv->IsRegistered(nick->GetNick(sID)) == 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :El nick no esta registrado." + "\r\n");
			return;
		} else if (server->HUBExiste() == 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :El HUB no existe, las BDs estan en modo de solo lectura." + "\r\n");
			return;
		} else if (datos->nicks[sID]->tiene_r == false) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :No te has identificado, para hacer DROP necesitas tener el nick puesto." + "\r\n");
			return;
		} else if (nickserv->Login(nick->GetNick(sID), x[1]) == 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :La password no coincide." + "\r\n");
			return;
		} else {
			string sql = "DELETE FROM NICKS WHERE NICKNAME='" + nick->GetNick(sID) + "';";
			if (db->SQLiteNoReturn(sql) == false) {
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :El nick " + nick->GetNick(sID) + " no ha sido borrado.\r\n");
				return;
			}
			sql = "DB " + db->GenerateID() + " " + sql;
			db->AlmacenaDB(sql);
			server->SendToAllServers(sql);
			sql = "DELETE FROM OPTIONS WHERE NICKNAME='" + nick->GetNick(sID) + "';";
			db->SQLiteNoReturn(sql);
			sql = "DB " + db->GenerateID() + " " + sql;
			db->AlmacenaDB(sql);
			server->SendToAllServers(sql); 
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :El nick " + nick->GetNick(sID) + " ha sido borrado.\r\n");
			if (datos->nicks[sID]->tiene_r == true) {
				sock->Write(stream, ":" + config->Getvalue("serverName") + " MODE " + nick->GetNick(sID) + " -r\r\n");
				datos->nicks[sID]->tiene_r = false;
			}
			return;
		}
	} else if (cmd == "EMAIL") {
		if (x.size() < 2) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :Necesito mas datos." + "\r\n");
			return;
		} else if (sID < 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return;
		} else if (nickserv->IsRegistered(nick->GetNick(sID)) == 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :El nick no esta registrado." + "\r\n");
			return;
		} else if (server->HUBExiste() == 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :El HUB no existe, las BDs estan en modo de solo lectura." + "\r\n");
			return;
		} else if (datos->nicks[sID]->tiene_r == false) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :No te has identificado, para hacer EMAIL necesitas tener el nick puesto." + "\r\n");
			return;
		} else {
			string email;
			if (mayus(x[1]) == "OFF") {
				email = "";
			} else {
				email = x[1];
			}
			string sql = "UPDATE NICKS SET EMAIL='" + email + "' WHERE NICKNAME='" + nick->GetNick(sID) + "';";
			if (db->SQLiteNoReturn(sql) == false) {
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :El nick " + nick->GetNick(sID) + " no ha podido cambiar el correo electronico.\r\n");
				return;
			}
			sql = "DB " + db->GenerateID() + " " + sql;
			db->AlmacenaDB(sql);
			server->SendToAllServers(sql);
			if (email.length() > 0)
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :Has cambiado tu EMAIL.\r\n");
			else
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :Has borrado tu EMAIL.\r\n");
			return;
		}
	} else if (cmd == "URL") {
		if (x.size() < 2) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :Necesito mas datos." + "\r\n");
			return;
		} else if (sID < 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return;
		} else if (nickserv->IsRegistered(nick->GetNick(sID)) == 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :El nick no esta registrado." + "\r\n");
			return;
		} else if (server->HUBExiste() == 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :El HUB no existe, las BDs estan en modo de solo lectura." + "\r\n");
			return;
		} else if (datos->nicks[sID]->tiene_r == false) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :No te has identificado, para hacer URL necesitas tener el nick puesto." + "\r\n");
			return;
		} else {
			string url;
			if (mayus(x[1]) == "OFF")
				url = "";
			else
				url = x[1];
			string sql = "UPDATE NICKS SET URL='" + url + "' WHERE NICKNAME='" + nick->GetNick(sID) + "';";
			if (db->SQLiteNoReturn(sql) == false) {
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :El nick " + nick->GetNick(sID) + " no ha podido cambiar la web.\r\n");
				return;
			}
			sql = "DB " + db->GenerateID() + " " + sql;
			db->AlmacenaDB(sql);
			server->SendToAllServers(sql);
			if (url.length() > 0)
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :Has cambiado tu URL.\r\n");
			else
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :Has borrado tu URL.\r\n");
			return;
		}
	} else if (cmd == "VHOST") {
		if (x.size() < 2) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :Necesito mas datos." + "\r\n");
			return;
		} else if (sID < 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return;
		} else if (nickserv->IsRegistered(nick->GetNick(sID)) == 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :El nick no esta registrado." + "\r\n");
			return;
		} else if (server->HUBExiste() == 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :El HUB no existe, las BDs estan en modo de solo lectura." + "\r\n");
			return;
		} else if (datos->nicks[sID]->tiene_r == false) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :No te has identificado, para hacer URL necesitas tener el nick puesto." + "\r\n");
			return;
		} else {
			string vHost;
			if (mayus(x[1]) == "OFF")
				vHost = "";
			else
				vHost = x[1];
			string sql = "UPDATE NICKS SET VHOST='" + vHost + "' WHERE NICKNAME='" + nick->GetNick(sID) + "';";
			if (db->SQLiteNoReturn(sql) == false) {
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :El nick " + nick->GetNick(sID) + " no ha podido cambiar la web.\r\n");
				return;
			}
			sql = "DB " + db->GenerateID() + " " + sql;
			db->AlmacenaDB(sql);
			server->SendToAllServers(sql);
			if (vHost.length() > 0)
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :Has cambiado tu VHOST.\r\n");
			else
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :Has borrado tu VHOST.\r\n");
			return;
		}
	}
	return;
}

bool NickServ::IsRegistered(string nickname) {
	string sql = "SELECT NICKNAME from NICKS WHERE NICKNAME='" + nickname + "';";
	string retorno = db->SQLiteReturnString(sql);
	if (retorno == nickname)
		return true;
	else
		return false;
} 

bool NickServ::Login (string nickname, string pass) {
	string sql = "SELECT PASS from NICKS WHERE NICKNAME='" + nickname + "';";
	string retorno = db->SQLiteReturnString(sql);
	if (retorno == sha256(pass))
		return true;
	else
		return false;
}

int NickServ::GetNicks () {
	string sql = "SELECT COUNT(*) FROM NICKS;";
	return db->SQLiteReturnInt(sql);
}
