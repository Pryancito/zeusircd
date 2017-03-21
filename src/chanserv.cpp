#include "include.h"

using namespace std;

ChanServ *chanserv = new ChanServ();

std::vector<std::string> split(const std::string &str, int delimiter(int) = ::isspace);

void ChanServ::ProcesaMensaje(TCPStream *stream, string mensaje) {
	if (mensaje.length() == 0 || mensaje == "\r\n" || mensaje == "\r" || mensaje == "\n" || mensaje == "||")
		return;
	vector<string> x = split(mensaje);
	string cmd = x[0];
	mayuscula(cmd);
	int sID = datos->BuscarIDStream(stream);
	
	if (cmd == "REGISTER") {
		if (x.size() < 2) {
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :Necesito mas datos." + "\r\n");
			return;
		} else if (sID < 0) {
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :No te has registrado." + "\r\n");
			return;
		} else if (chanserv->IsRegistered(x[1]) == 1) {
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El canal ya esta registrado." + "\r\n");
			return;
		} else if (nickserv->IsRegistered(nick->GetNick(sID)) == 0) {
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :Necesitas tener el nick registrado para registrar un canal." + "\r\n");
			return;
		} else if (server->HUBExiste() == 0) {
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El HUB no existe, las BDs estan en modo de solo lectura." + "\r\n");
			return;
		} else if (chan->IsInChan(x[1], nick->GetNick(sID)) == 0) {
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :Necesitas estar dentro del canal para registrarlo." + "\r\n");
			return;
		} else {
			string sql = "INSERT INTO CANALES VALUES ('" + x[1] + "', '" + nick->GetNick(sID) + "', '+r', 'El Canal ha sido registrado',  " + to_string(time(0)) + ", " + to_string(time(0)) + ");";
			if (db->SQLiteNoReturn(sql) == false) {
				sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El canal " + x[1] + " no ha sido registrado.\r\n");
				return;
			}
			sql = "DB " + db->GenerateID() + " " + sql;
			db->AlmacenaDB(sql);
			server->SendToAllServers(sql);
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El canal " + x[1] + " ha sido registrado.\r\n");
			int id = datos->GetChanPosition(x[1]);
			if (id >= 0) {
				if (datos->canales[id]->tiene_r == false) {
					datos->canales[id]->tiene_r = true;
					chan->PropagarMODE("CHaN!*@*", "", x[1], 'r', 1);
				}
				chan->PropagarMODE("CHaN!*@*", nick->GetNick(sID), x[1], 'o', 1);
			}
			return;
		}
	} else if (cmd == "DROP") {
		if (x.size() < 2) {
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :Necesito mas datos." + "\r\n");
			return;
		} else if (sID < 0) {
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :No te has registrado." + "\r\n");
			return;
		} else if (nickserv->IsRegistered(nick->GetNick(sID)) == 0) {
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :Tu nick no esta registrado." + "\r\n");
			return;
		} else if (server->HUBExiste() == 0) {
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El HUB no existe, las BDs estan en modo de solo lectura." + "\r\n");
			return;
		} else if (datos->nicks[sID]->tiene_r == false) {
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :No te has identificado, para hacer DROP necesitas tener el nick puesto." + "\r\n");
			return;
		} else if (chan->IsInChan(x[1], nick->GetNick(sID)) == 0) {
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :Necesitas estar dentro del canal para hacer DROP." + "\r\n");
			return;
		} else if (chanserv->IsFounder(nick->GetNick(sID), x[1]) == 0) {
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :No eres el fundador del canal." + "\r\n");
			return;
		} else {
			string sql = "DELETE FROM CANALES WHERE NOMBRE='" + x[1] + "' COLLATE NOCASE;";
			if (db->SQLiteNoReturn(sql) == false) {
				sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El canal " + x[1] + " no ha sido borrado.\r\n");
				return;
			}
			sql = "DB " + db->GenerateID() + " " + sql;
			db->AlmacenaDB(sql);
			server->SendToAllServers(sql);
			sql = "DELETE FROM ACCESS WHERE CANAL='" + x[1] + "' COLLATE NOCASE;";
			db->SQLiteNoReturn(sql);
			sql = "DB " + db->GenerateID() + " " + sql;
			db->AlmacenaDB(sql);
			server->SendToAllServers(sql);
			sql = "DELETE FROM AKICK WHERE CANAL='" + x[1] + "' COLLATE NOCASE;";
			db->SQLiteNoReturn(sql);
			sql = "DB " + db->GenerateID() + " " + sql;
			db->AlmacenaDB(sql);
			server->SendToAllServers(sql);
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El canal " + x[1] + " ha sido borrado.\r\n");
			int id = datos->GetChanPosition(x[1]);
			if (id >= 0) {
				if (datos->canales[id]->tiene_r == true) {
					datos->canales[id]->tiene_r = false;
					chan->PropagarMODE("CHaN!*@*", "", x[1], 'r', 0);
				}
				chan->PropagarMODE("CHaN!*@*", nick->GetNick(sID), x[1], 'o', 0);
			}
			return;
		}
	} else if (cmd == "VOP" || cmd == "HOP" || cmd == "AOP" || cmd == "SOP") {
		if (x.size() < 4) {
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :Necesito mas datos." + "\r\n");
			return;
		} else if (sID < 0) {
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :No te has registrado." + "\r\n");
			return;
		} else if (nickserv->IsRegistered(nick->GetNick(sID)) == 0) {
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :Tu nick no esta registrado." + "\r\n");
			return;
		} else if (chanserv->IsRegistered(x[1]) == 0) {
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El canal no esta registrado." + "\r\n");
			return;
		} else if (server->HUBExiste() == 0) {
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El HUB no existe, las BDs estan en modo de solo lectura." + "\r\n");
			return;
		} else if (datos->nicks[sID]->tiene_r == false) {
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :No te has identificado, para manejar las listas de acceso necesitas tener el nick puesto." + "\r\n");
			return;
		} else if (chanserv->Access(nick->GetNick(sID), x[1]) < 4) {
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :No tienes suficiente acceso." + "\r\n");
			return;
		} else if (nickserv->GetOption("NOACCESS", nick->GetNick(sID)) == 1) {
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El nick tiene activada la opcion NOACCESS." + "\r\n");
			return;
		} else {
			if (mayus(x[2]) == "ADD") {
				if (chanserv->Access(x[3], x[1]) != 0) {
					string sql = "UPDATE ACCESS SET ACCESO='" + mayus(cmd) + "' FROM ACCESS WHERE NOMBRE='" + x[1] + "' COLLATE NOCASE;";
					if (db->SQLiteNoReturn(sql) == false) {
						sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El registro no se ha podido insertar.\r\n");
						return;
					}
					sql = "DB " + db->GenerateID() + " " + sql;
					db->AlmacenaDB(sql);
					server->SendToAllServers(sql);
				} else {
					string sql = "INSERT INTO ACCESS VALUES ('" + x[1] + "', '" + mayus(cmd) + "', '" + x[3] + "', '" + nick->GetNick(sID) + "');";
					if (db->SQLiteNoReturn(sql) == false) {
						sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El registro no se ha podido insertar.\r\n");
						return;
					}
					sql = "DB " + db->GenerateID() + " " + sql;
					db->AlmacenaDB(sql);
					server->SendToAllServers(sql);
				}
				sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :Se ha dado " + cmd + " a " + x[3] + "\r\n");
			} else if (mayus(x[2]) == "DEL") {
				if (chanserv->Access(x[3], x[1]) == 0) {
					sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El usuario no tiene acceso.\r\n");
					return;
				}
				string sql = "DELETE FROM ACCESS WHERE USUARIO='" + x[3] + "' AND CANAL='" + x[1] + "' AND ACCESO='" + mayus(cmd) + "' COLLATE NOCASE;";
				if (db->SQLiteNoReturn(sql) == false) {
					sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El registro no se ha podido borrar.\r\n");
					return;
				}
				sql = "DB " + db->GenerateID() + " " + sql;
				db->AlmacenaDB(sql);
				server->SendToAllServers(sql);
				sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :Se ha quitado " + cmd + " a " + x[3] + "\r\n");
			}
			chanserv->CheckModes(x[3], x[1]);
			return;
		}
	}
}

void ChanServ::CheckModes(string nickname, string channel) {
	int id = datos->GetChanPosition(channel);
	int pos = datos->GetNickPosition(channel, nickname);
	if (id >= 0 && pos >= 0) {
		int access = chanserv->Access(nickname, channel);
		if (datos->canales[id]->umodes[pos] == 'x') {
			if (access == 1) {
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'v', 1);
				datos->canales[id]->umodes[pos] = 'v';
			} else if (access == 2) {
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'h', 1);
				datos->canales[id]->umodes[pos] = 'h';
			} else if (access >= 3) {
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'o', 1);
				datos->canales[id]->umodes[pos] = 'o';
			}
		} else if (datos->canales[id]->umodes[pos] == 'v') {
			if (access == 2) {
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'v', 0);
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'h', 1);
				datos->canales[id]->umodes[pos] = 'h';
			} else if (access >= 3) {
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'v', 0);
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'o', 1);
				datos->canales[id]->umodes[pos] = 'o';
			}
		} else if (datos->canales[id]->umodes[pos] == 'h') {
			if (access == 1) {
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'h', 0);
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'v', 1);
				datos->canales[id]->umodes[pos] = 'v';
			} else if (access >= 3) {
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'h', 0);
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'o', 1);
				datos->canales[id]->umodes[pos] = 'o';
			}
		} else if (datos->canales[id]->umodes[pos] == 'o') {
			if (access == 1) {
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'o', 0);
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'v', 1);
				datos->canales[id]->umodes[pos] = 'v';
			} else if (access == 2) {
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'o', 0);
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'h', 1);
				datos->canales[id]->umodes[pos] = 'h';
			}
		}
	}
	return;
}

bool ChanServ::IsRegistered(string channel) {
	string sql = "SELECT NOMBRE from CANALES WHERE NOMBRE='" + channel + "' COLLATE NOCASE;";
	string retorno = db->SQLiteReturnString(sql);
	if (mayus(retorno) == mayus(channel))
		return true;
	else
		return false;
}

bool ChanServ::IsFounder(string nickname, string channel) {
	string sql = "SELECT OWNER from CANALES WHERE NOMBRE='" + channel + "' COLLATE NOCASE;";
	string retorno = db->SQLiteReturnString(sql);
	if (mayus(retorno) == mayus(nickname))
		return true;
	else
		return false;
}

int ChanServ::Access (string nickname, string channel) {
	string sql = "SELECT ACCESO from ACCESS WHERE USUARIO='" + nickname + "' AND CANAL='" + channel + "' COLLATE NOCASE;";
	string retorno = db->SQLiteReturnString(sql);
	if (mayus(retorno) == "VOP")
		return 1;
	else if (mayus(retorno) == "HOP")
		return 2;
	else if (mayus(retorno) == "AOP")
		return 3;
	else if (mayus(retorno) == "SOP")
		return 4;
	else if (chanserv->IsFounder(nickname, channel) == 1)
		return 5;
	else return 0;
}

int ChanServ::GetChans () {
	string sql = "SELECT COUNT(*) FROM CANALES;";
	return db->SQLiteReturnInt(sql);
}
