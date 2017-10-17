#include "include.h"
#include <regex>

using namespace std;

OperServ *operserv = new OperServ();

void insert_rule (string ip)
{
	string cmd;
	for (unsigned int i = 0; config->Getvalue("listen["+boost::to_string(i)+"]port").length() > 0; i++) {
		cmd = "sudo iptables -A INPUT -s " + ip + " -d " + config->Getvalue("listen["+boost::to_string(i)+"]ip") + " -p tcp --dport " + config->Getvalue("listen["+boost::to_string(i)+"]port") + " -j REJECT";
		system(cmd.c_str());
	}
	for (unsigned int i = 0; config->Getvalue("listen6["+boost::to_string(i)+"]port").length() > 0; i++) {
		cmd = "sudo ip6tables -A INPUT -s " + ip + " -d " + config->Getvalue("listen6["+boost::to_string(i)+"]ip") + " -p tcp --dport " + config->Getvalue("listen6["+boost::to_string(i)+"]port") + " -j REJECT";
		system(cmd.c_str());
	}
}

void delete_rule (string ip)
{
	string cmd;
	for (unsigned int i = 0; config->Getvalue("listen["+boost::to_string(i)+"]port").length() > 0; i++) {
		cmd = "sudo iptables -D INPUT -s " + ip + " -d " + config->Getvalue("listen["+boost::to_string(i)+"]ip") + " -p tcp --dport " + config->Getvalue("listen["+boost::to_string(i)+"]port") + " -j REJECT";
		system(cmd.c_str());
	} for (unsigned int i = 0; config->Getvalue("listen6["+boost::to_string(i)+"]port").length() > 0; i++) {
		cmd = "sudo ip6tables -D INPUT -s " + ip + " -d " + config->Getvalue("listen6["+boost::to_string(i)+"]ip") + " -p tcp --dport " + config->Getvalue("listen6["+boost::to_string(i)+"]port") + " -j REJECT";
		system(cmd.c_str());
	}
}

void OperServ::ApplyGlines () {
	string cmd = "sudo iptables -F INPUT";
	system(cmd.c_str());
	vector <string> ip;
	string sql = "SELECT IP FROM GLINE";
	ip = db->SQLiteReturnVector(sql);
	for (unsigned int i = 0; i < ip.size(); i++)
		insert_rule(ip[i]);
}

void OperServ::ProcesaMensaje(Socket *s, User *u, string mensaje) {
	if (mensaje.length() == 0 || mensaje == "\r\n" || mensaje == "\r" || mensaje == "\n")
		return;
	vector<string> x;
	boost::split(x,mensaje,boost::is_any_of(" "));
	string cmd = x[0];
	mayuscula(cmd);
	
	if (cmd == "HELP") {
		s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :[ /operserv gline|kill|drop ]" + "\r\n");
		return;
	} else if (cmd == "GLINE") {
		if (x.size() < 2) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :Necesito mas datos. [ /operserv gline add|del|list (ip) (motivo) ]" + "\r\n");
			return;
		} else if (u->GetReg() == false) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :No te has registrado." + "\r\n");
			return;
		} else if (nickserv->IsRegistered(u->GetNick()) == 0) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :Tu nick no esta registrado." + "\r\n");
			return;
		} else if (server->HUBExiste() == 0) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + "\r\n");
			return;
		} else if (u->Tiene_Modo('r') == false) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :No te has identificado, para manejar las listas de acceso necesitas tener el nick puesto." + "\r\n");
			return;
		} else {
			if (boost::iequals(x[1], "ADD")) {
				mayuscula(cmd);
				if (x.size() < 4) {
					s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :Necesito mas datos." + "\r\n");
					return;
				} else if (operserv->IsGlined(x[2]) == 1) {
					s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :El GLINE ya existe." + "\r\n");
					return;
				}
				int length = 7 + x[1].length() + x[2].length();
				string motivo = mensaje.substr(length);
				string sql = "INSERT INTO GLINE VALUES ('" + x[2] + "', '" + motivo + "', '" + u->GetNick() + "');";
				if (db->SQLiteNoReturn(sql) == false) {
					s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :El registro no se ha podido insertar.\r\n");
					return;
				}
				sql = "DB " + db->GenerateID() + " " + sql;
				db->AlmacenaDB(sql);
				server->SendToAllServers(sql);
				for (User *usr = users.first(); usr != NULL; usr = users.next(usr))
					if (boost::iequals(usr->GetIP(), x[2])) {
						Socket *sok = user->GetSocket(usr->GetNick());
						vector <UserChan*> temp;
						for (UserChan *uc = usuarios.first(); uc != NULL; uc = usuarios.next(uc)) {
							if (boost::iequals(uc->GetID(), usr->GetID(), loc)) {
								chan->PropagarQUIT(usr, uc->GetNombre());
								temp.push_back(uc);
							}
						}
						for (unsigned int i = 0; i < temp.size(); i++) {
							UserChan *uc = temp[i];
							usuarios.del(uc);
							if (chan->GetUsers(uc->GetNombre()) == 0) {
								chan->DelChan(uc->GetNombre());
							}
						}
						users.del(usr);
						sok->Close();
						sock.del(sok);
					}
				insert_rule(x[2]);
				oper->GlobOPs("Se ha insertado el GLINE a la IP " + x[2] + " por " + u->GetNick() + ". Motivo: " + motivo + ".\r\n");
			} else if (boost::iequals(x[1], "DEL")) {
				if (x.size() < 3) {
					s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :Necesito mas datos." + "\r\n");
					return;
				}
				if (operserv->IsGlined(x[2]) == 0) {
					s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :No existe GLINE con esa IP." + "\r\n");
					return;
				}
				string sql = "DELETE FROM GLINE WHERE IP='" + x[2] + "' COLLATE NOCASE;";
				if (db->SQLiteNoReturn(sql) == false) {
					s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :El registro no se ha podido borrar." + "\r\n");
					return;
				}
				sql = "DB " + db->GenerateID() + " " + sql;
				db->AlmacenaDB(sql);
				server->SendToAllServers(sql);
				delete_rule(x[2]);
				s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :Se ha quitado la GLINE." + "\r\n");
			} else if (boost::iequals(x[1], "LIST")) {
				vector <string> ip;
				vector <string> who;
				vector <string> motivo;
				string sql = "SELECT IP FROM GLINE;";
				ip = db->SQLiteReturnVector(sql);
				sql = "SELECT NICK FROM GLINE;";
				who = db->SQLiteReturnVector(sql);
				sql = "SELECT MOTIVO FROM GLINE;";
				motivo = db->SQLiteReturnVector(sql);
				if (ip.size() == 0)
					s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :No hay GLINES.\r\n");
				for (unsigned int i = 0; i < ip.size(); i++) {
					s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :\002" + ip[i] + "\002 por " + who[i] + ". Motivo: " + motivo[i] + ".\r\n");
				}
				return;
			}
			return;
		}
	} else if (cmd == "KILL") {
		if (x.size() < 2) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :Necesito mas datos. [ /operserv kill (nick) ]" + "\r\n");
			return;
		} else if (u->GetReg() == false) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :No te has registrado." + "\r\n");
			return;
		} else if (user->GetUserByNick(x[1]) == NULL) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :El nick no esta conectado." + "\r\n");
			return;
		} else if (user->GetSocket(x[1]) != NULL) {
			User *us = user->GetUserByNick(x[1]);
			Socket *sck = user->GetSocket(x[1]);
			vector <UserChan*> temp;
			for (UserChan *uc = usuarios.first(); uc != NULL; uc = usuarios.next(uc)) {
				if (boost::iequals(uc->GetID(), us->GetID(), loc)) {
					chan->PropagarQUIT(us, uc->GetNombre());
					temp.push_back(uc);
				}
			}
			for (unsigned int i = 0; i < temp.size(); i++) {
				UserChan *uc = temp[i];
				usuarios.del(uc);
				if (chan->GetUsers(uc->GetNombre()) == 0) {
					chan->DelChan(uc->GetNombre());
				}
			}
			for (User *usr = users.first(); usr != NULL; usr = users.next(usr))
				if (boost::iequals(usr->GetID(), us->GetID(), loc)) {
					users.del(usr);
					server->SendToAllServers("QUIT " + usr->GetID());
					break;
				}
			for (Socket *socket = sock.first(); socket != NULL; socket = sock.next(socket))
				if (boost::iequals(socket->GetID(), sck->GetID(), loc)) {
					socket->Close();
					sock.del(socket);
					break;
				}
			return;
		} else {
			User *us = user->GetUserByNick(x[1]);
			server->SendToAllServers("QUIT " + us->GetID());
			return;
		}
	} else if (cmd == "DROP") {
		if (x.size() < 2) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :Necesito mas datos. [ /operserv drop (nick|canal) ]" + "\r\n");
			return;
		} else if (u->GetReg() == false) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :No te has registrado." + "\r\n");
			return;
		} else if (nickserv->IsRegistered(x[1]) == 0 && chanserv->IsRegistered(x[1]) == 0) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :El nick no esta registrado." + "\r\n");
			return;
		} else if (server->HUBExiste() == 0) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + "\r\n");
			return;
		} else if (u->Tiene_Modo('r') == false) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :No te has identificado, para hacer DROP necesitas tener el nick puesto." + "\r\n");
			return;
		} else if (nickserv->IsRegistered(x[1]) == 1) {
			string sql = "DELETE FROM NICKS WHERE NICKNAME='" + x[1] + "' COLLATE NOCASE;";
			if (db->SQLiteNoReturn(sql) == false) {
				s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :El nick " + x[1] + " no ha sido borrado.\r\n");
				return;
			}
			sql = "DB " + db->GenerateID() + " " + sql;
			db->AlmacenaDB(sql);
			server->SendToAllServers(sql);
			sql = "DELETE FROM OPTIONS WHERE NICKNAME='" + x[1] + "' COLLATE NOCASE;";
			db->SQLiteNoReturn(sql);
			sql = "DB " + db->GenerateID() + " " + sql;
			db->AlmacenaDB(sql);
			server->SendToAllServers(sql);
			sql = "DELETE FROM ACCESS WHERE USUARIO='" + x[1] + "' COLLATE NOCASE;";
			db->SQLiteNoReturn(sql);
			sql = "DB " + db->GenerateID() + " " + sql;
			db->AlmacenaDB(sql);
			server->SendToAllServers(sql);
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :El nick " + x[1] + " ha sido borrado.\r\n");
			return;
		} else if (chanserv->IsRegistered(x[1]) == 1) {
			string sql = "DELETE FROM CANALES WHERE NOMBRE='" + x[1] + "' COLLATE NOCASE;";
			if (db->SQLiteNoReturn(sql) == false) {
				s->Write(":CHaN!*@* NOTICE " + u->GetNick() + " :El canal " + x[1] + " no ha sido borrado.\r\n");
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
			s->Write(":CHaN!*@* NOTICE " + u->GetNick() + " :El canal " + x[1] + " ha sido borrado.\r\n");
			for (Chan *canal = canales.first(); canal != NULL; canal = canales.next(canal))
				if (boost::iequals(canal->GetNombre(), x[1], loc)) {
					if (canal != NULL) {
						if (canal->Tiene_Modo('r') == true) {
							canal->Fijar_Modo('r', false);
							chan->PropagarMODE("CHaN!*@*", "", x[1], 'r', 0, 1);
						}
					}
				}
		}
	}
}

bool OperServ::IsGlined(string ip) {
	string sql = "SELECT IP from GLINE WHERE IP='" + ip + "' COLLATE NOCASE;";
	string retorno = db->SQLiteReturnString(sql);
	if (boost::iequals(retorno, ip, loc))
		return true;
	else
		return false;
}
