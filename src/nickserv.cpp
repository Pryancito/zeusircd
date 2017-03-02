#include "include.h"
#include "sha256.h"

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
			cout << 1 << endl;
			string sql = "INSERT INTO NICKS VALUES ( '" + nick->GetNick(sID) + "', '" + sha256(x[1]) + "', NULL, NULL, NULL,  " + to_string(time(0)) + ", " + to_string(time(0)) + " );";
			if (db->SQLiteNoReturn(sql) == false) {
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :El nick " + nick->GetNick(sID) + " no ha sido registrado.\r\n");
				return;
			}
			cout << 2 << endl;
			sql = "DB " + db->GenerateID() + " " + sql;
			db->AlmacenaDB(sql);
			server->SendToAllServers(sql);
			cout << 3 << endl;
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :El nick " + nick->GetNick(sID) + " ha sido registrado.\r\n");
			if (datos->nicks[sID]->tiene_r == false) {
				sock->Write(stream, ":" + config->Getvalue("serverName") + " MODE " + nick->GetNick(sID) + " +r\r\n");
				datos->nicks[sID]->tiene_r = true;
			}
			cout << 4 << endl;
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
