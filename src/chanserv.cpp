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
		if (x.size() < 3) {
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
				if (x.size() < 4) {
					sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :Necesito mas datos." + "\r\n");
					return;
				} else if (nickserv->IsRegistered(x[3]) == 0) {
					sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El nick no esta registrado." + "\r\n");
					return;
				}
				if (chanserv->Access(x[3], x[1]) != 0) {
					string sql = "UPDATE ACCESS SET ACCESO='" + mayus(cmd) + "' FROM ACCESS WHERE NOMBRE='" + x[1] + "' COLLATE NOCASE;";
					if (db->SQLiteNoReturn(sql) == false) {
						sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El registro no se ha podido insertar.\r\n");
						return;
					}
					sql = "DB " + db->GenerateID() + " " + sql;
					db->AlmacenaDB(sql);
					server->SendToAllServers(sql);
					sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :Se ha cambiado el registro.\r\n");
				} else {
					string sql = "INSERT INTO ACCESS VALUES ('" + x[1] + "', '" + mayus(cmd) + "', '" + x[3] + "', '" + nick->GetNick(sID) + "');";
					if (db->SQLiteNoReturn(sql) == false) {
						sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El registro no se ha podido insertar.\r\n");
						return;
					}
					sql = "DB " + db->GenerateID() + " " + sql;
					db->AlmacenaDB(sql);
					server->SendToAllServers(sql);
					sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :Se ha insertado el registro.\r\n");
					chanserv->CheckModes(x[3], x[1]);
				}
				sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :Se ha dado " + cmd + " a " + x[3] + "\r\n");
			} else if (mayus(x[2]) == "DEL") {
				if (x.size() < 4) {
					sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :Necesito mas datos." + "\r\n");
					return;
				}
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
			} else if (mayus(x[2]) == "LIST") {
				vector <string> usuarios;
				vector <string> who;
				string sql = "SELECT USUARIO FROM ACCESS WHERE CANAL='" + x[1] + "' AND ACCESO='" + mayus(cmd) + "' COLLATE NOCASE;";
				usuarios = db->SQLiteReturnVector(sql);
				sql = "SELECT ADDED FROM ACCESS WHERE CANAL='" + x[1] + "' AND ACCESO='" + mayus(cmd) + "' COLLATE NOCASE;";
				who = db->SQLiteReturnVector(sql);
				if (usuarios.size() == 0)
					sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :No hay accesos de " + cmd + ".\r\n");
				for (unsigned int i = 0; i < usuarios.size(); i++) {
					sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :\002" + usuarios[i] + "\002 accesado por " + who[i] + ".\r\n");
				}
			}
			return;
		}
	} else if (cmd == "TOPIC") {
		if (x.size() < 3) {
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
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :No te has identificado, para cambiar el topic necesitas tener el nick puesto." + "\r\n");
			return;
		} else if (chanserv->IsRegistered(x[1]) == 0) {
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El canal no esta registrado." + "\r\n");
			return;
		} else if (chanserv->Access(nick->GetNick(sID), x[1]) < 3) {
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :No tienes acceso para cambiar el topic." + "\r\n");
			return;
		} else {
			int pos = 7 + x[1].length();
			string topic = mensaje.substr(pos);
			if (topic.find(";") != std::string::npos || topic.find("'") != std::string::npos || topic.find("\"") != std::string::npos) {
				sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El topic contiene caracteres no validos." + "\r\n");
				return;
			}
			if (topic.length() > 255) {
				sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El topic es demasiado largo." + "\r\n");
				return;
			}
			string sql = "UPDATE CANALES SET TOPIC='" + topic + "' WHERE NOMBRE='" + x[1] + "' COLLATE NOCASE;";
			if (db->SQLiteNoReturn(sql) == false) {
				sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El topic no se ha podido cambiar.\r\n");
				return;
			}
			sql = "DB " + db->GenerateID() + " " + sql;
			db->AlmacenaDB(sql);
			server->SendToAllServers(sql);
			TCPStream *streamnick;
			int id = datos->GetChanPosition(x[1]);
			if (id < 0)
				return;
			lock_guard<std::mutex> lock(nick_mute);
			for (unsigned int i = 0; i < datos->canales[id]->usuarios.size(); i++) {
				streamnick = datos->BuscarStream(datos->canales[id]->usuarios[i]->nombre);
				if (streamnick != NULL)
					sock->Write(streamnick, ":CHaN!*@* 332 " + datos->canales[id]->usuarios[i]->nombre + " " + datos->canales[id]->nombre + " :" + topic + "\r\n");
			}
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El topic se ha cambiado.\r\n");
			return;
		}
	} else if (cmd == "AKICK") {
		if (x.size() < 3) {
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
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :No te has identificado, para manejar los AKICK necesitas tener el nick puesto." + "\r\n");
			return;
		} else if (chanserv->IsRegistered(x[1]) == 0) {
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El canal no esta registrado." + "\r\n");
			return;
		} else if (chanserv->Access(nick->GetNick(sID), x[1]) < 4) {
			sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :No tienes acceso para cambiar los AKICK." + "\r\n");
			return;
		} else {
			if (mayus(x[2]) == "ADD") {
				if (x.size() < 5) {
					sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :Necesito mas datos." + "\r\n");
					return;
				}
				if (chanserv->IsAKICK(x[3], x[1]) != 0) {
					sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :La mascara ya tiene AKICK.\r\n");
					return;
				} else {
					int posicion = 4 + cmd.length() + x[1].length() + x[2].length() + x[3].length();
					string motivo = mensaje.substr(posicion);
					if (motivo.find(";") != std::string::npos || motivo.find("'") != std::string::npos || motivo.find("\"") != std::string::npos) {
						sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El motivo contiene caracteres no validos." + "\r\n");
						return;
					}
					string sql = "INSERT INTO AKICK VALUES ('" + x[1] + "', '" + x[3] + "', '" + motivo + "', '" + nick->GetNick(sID) + "');";
					if (db->SQLiteNoReturn(sql) == false) {
						sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El AKICK no se ha podido insertar.\r\n");
						return;
					}
					sql = "DB " + db->GenerateID() + " " + sql;
					db->AlmacenaDB(sql);
					server->SendToAllServers(sql);
					sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :Se ha insertado el AKICK.\r\n");
				}
			} else if (mayus(x[2]) == "DEL") {
				if (x.size() < 4) {
					sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :Necesito mas datos." + "\r\n");
					return;
				}
				if (chanserv->IsAKICK(x[3], x[1]) == 0) {
					sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El usuario no tiene AKICK.\r\n");
					return;
				}
				string sql = "DELETE FROM AKICK WHERE MASCARA='" + x[3] + "' AND CANAL='" + x[1] + "' COLLATE NOCASE;";
				if (db->SQLiteNoReturn(sql) == false) {
					sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El AKICK no se ha podido borrar.\r\n");
					return;
				}
				sql = "DB " + db->GenerateID() + " " + sql;
				db->AlmacenaDB(sql);
				server->SendToAllServers(sql);
				sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :Se ha quitado " + cmd + " a " + x[3] + "\r\n");
			} else if (mayus(x[2]) == "LIST") {
				vector <string> akick;
				vector <string> who;
				string sql = "SELECT MASCARA FROM AKICK WHERE CANAL='" + x[1] + "' COLLATE NOCASE;";
				akick = db->SQLiteReturnVector(sql);
				sql = "SELECT ADDED FROM AKICK WHERE CANAL='" + x[1] + "' COLLATE NOCASE;";
				who = db->SQLiteReturnVector(sql);
				if (akick.size() == 0)
					sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :No hay AKICKS en " + x[1] + "\r\n");
				for (unsigned int i = 0; i < akick.size(); i++) {
					sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :\002" + akick[i] + "\002 realizado por " + who[i] + ".\r\n");
				}
			}
			return;
		}
	}
}

void ChanServ::CheckModes(string nickname, string channel) {
	int id = datos->GetChanPosition(channel);
	int pos = datos->GetNickPosition(channel, nickname);
	if (id > -1 && pos > -1) {
		int access = chanserv->Access(nickname, channel);
		if (datos->canales[id]->usuarios[pos]->modo == 'v') {
			if (access == 2) {
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'v', 0);
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'h', 1);
				datos->canales[id]->usuarios[pos]->modo = 'h';
			} else if (access > 2) {
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'v', 0);
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'o', 1);
				datos->canales[id]->usuarios[pos]->modo = 'o';
			}
		} else if (datos->canales[id]->usuarios[pos]->modo == 'h') {
			if (access == 1) {
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'h', 0);
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'v', 1);
				datos->canales[id]->usuarios[pos]->modo = 'v';
			} else if (access > 2) {
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'h', 0);
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'o', 1);
				datos->canales[id]->usuarios[pos]->modo = 'o';
			}
		} else if (datos->canales[id]->usuarios[pos]->modo == 'o') {
			if (access == 1) {
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'o', 0);
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'v', 1);
				datos->canales[id]->usuarios[pos]->modo = 'v';
			} else if (access == 2) {
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'o', 0);
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'h', 1);
				datos->canales[id]->usuarios[pos]->modo = 'h';
			}
		} else {
			if (access == 1) {
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'v', 1);
				datos->canales[id]->usuarios[pos]->modo = 'v';
			} else if (access == 2) {
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'h', 1);
				datos->canales[id]->usuarios[pos]->modo = 'h';
			} else if (access > 2) {
				chan->PropagarMODE("CHaN!*@*", nickname, channel, 'o', 1);
				datos->canales[id]->usuarios[pos]->modo = 'o';
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
	else if (chanserv->IsFounder(nickname, channel) == 1 || oper->IsOper(nickname) == 1)
		return 5;
	else return 0;
}

bool ChanServ::IsAKICK(string mascara, string canal) {
	vector <string> akicks;
	string sql = "SELECT MASCARA from AKICK WHERE CANAL='" + canal + "' COLLATE NOCASE;";
	akicks = db->SQLiteReturnVector(sql);
	for (unsigned int i = 0; i < akicks.size(); i++) {
		if (datos->Match(akicks[i].c_str(), mascara.c_str()) == 1)
			return true;
	}
	return false;
}

int ChanServ::GetChans () {
	string sql = "SELECT COUNT(*) FROM CANALES;";
	return db->SQLiteReturnInt(sql);
}
