#include "include.h"
#include "sha256.h"

using namespace std;


std::string OPERSERV;

const char *OperServ::pseudoClient(void)
{
	/** Fuck ISO C++ */
	OPERSERV = "OPeR!-@-";
	return OPERSERV.c_str();
}
void insert_rule (string ip)
{
	string cmd;
	for (unsigned int i = 0; config->Getvalue("listen["+boost::to_string(i)+"]port").length() > 0; i++) {
		cmd = "sudo iptables -A INPUT -s " + ip + " -d " + config->Getvalue("listen["+boost::to_string(i)+"]ip") + " -p tcp --dport " + config->Getvalue("listen["+boost::to_string(i)+"]port") + " -j DROP";
		system(cmd.c_str());
	}
	for (unsigned int i = 0; config->Getvalue("listen6["+boost::to_string(i)+"]port").length() > 0; i++) {
		cmd = "sudo ip6tables -A INPUT -s " + ip + " -d " + config->Getvalue("listen6["+boost::to_string(i)+"]ip") + " -p tcp --dport " + config->Getvalue("listen6["+boost::to_string(i)+"]port") + " -j DROP";
		system(cmd.c_str());
	}
}

void delete_rule (string ip)
{
	string cmd;
	for (unsigned int i = 0; config->Getvalue("listen["+boost::to_string(i)+"]port").length() > 0; i++) {
		cmd = "sudo iptables -D INPUT -s " + ip + " -d " + config->Getvalue("listen["+boost::to_string(i)+"]ip") + " -p tcp --dport " + config->Getvalue("listen["+boost::to_string(i)+"]port") + " -j DROP";
		system(cmd.c_str());
	} for (unsigned int i = 0; config->Getvalue("listen6["+boost::to_string(i)+"]port").length() > 0; i++) {
		cmd = "sudo ip6tables -D INPUT -s " + ip + " -d " + config->Getvalue("listen6["+boost::to_string(i)+"]ip") + " -p tcp --dport " + config->Getvalue("listen6["+boost::to_string(i)+"]port") + " -j DROP";
		system(cmd.c_str());
	}
}

void OperServ::ApplyGlines () {
	string cmd = "sudo iptables -F INPUT";
	system(cmd.c_str());
	vector <string> ip;
	string sql = "SELECT IP FROM GLINE";
	ip = DB::SQLiteReturnVector(sql);
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
		s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :[ /operserv gline|kill|drop|setpass ]" + "\r\n");
		return;
	} else if (cmd == "GLINE") {
		if (x.size() < 2) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :Necesito mas datos. [ /operserv gline add|del|list (ip) (motivo) ]" + "\r\n");
			return;
		} else if (u->GetReg() == false) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :No te has registrado." + "\r\n");
			return;
		} else if (NickServ::IsRegistered(u->GetNick()) == 0) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :Tu nick no esta registrado." + "\r\n");
			return;
		} else if (Servidor::HUBExiste() == 0) {
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
				} else if (OperServ::IsGlined(x[2]) == 1) {
					s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :El GLINE ya existe." + "\r\n");
					return;
				} else if (x[2].find(";") != std::string::npos || x[2].find("'") != std::string::npos || x[2].find("\"") != std::string::npos) {
					s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :El GLINE contiene caracteres no validos." + "\r\n");
					return;
				}
				int length = 7 + x[1].length() + x[2].length();
				string motivo = mensaje.substr(length);
				string sql = "INSERT INTO GLINE VALUES ('" + x[2] + "', '" + motivo + "', '" + u->GetNick() + "');";
				if (DB::SQLiteNoReturn(sql) == false) {
					s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :El registro no se ha podido insertar.\r\n");
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::SendToAllServers(sql);	
				for (auto it = users.begin(); it != users.end(); it++)
					if (boost::iequals((*it)->GetIP(), x[2])) {
						for (auto it2 = (*it)->channels.begin(); it2 != (*it)->channels.end();) {
							Chan::PropagarQUIT(*it, (*it2)->GetNombre());
							Chan::Part(*it, (*it2)->GetNombre());
							it2 = (*it)->channels.erase(it2);
						}
						(*it)->GetSocket()->Close();
						users.erase(it);
						Servidor::SendToAllServers("QUIT " + (*it)->GetID());
						break;
					}
				insert_rule(x[2]);
				Servidor::SendToAllServers("NEWGLINE");
				Oper::GlobOPs("Se ha insertado el GLINE a la IP " + x[2] + " por " + u->GetNick() + ". Motivo: " + motivo + ".\r\n");
			} else if (boost::iequals(x[1], "DEL")) {
				if (x.size() < 3) {
					s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :Necesito mas datos." + "\r\n");
					return;
				}
				if (OperServ::IsGlined(x[2]) == 0) {
					s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :No existe GLINE con esa IP." + "\r\n");
					return;
				}
				string sql = "DELETE FROM GLINE WHERE IP='" + x[2] + "' COLLATE NOCASE;";
				if (DB::SQLiteNoReturn(sql) == false) {
					s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :El registro no se ha podido borrar." + "\r\n");
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::SendToAllServers(sql);
				delete_rule(x[2]);
				Servidor::SendToAllServers("NEWGLINE");
				s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :Se ha quitado la GLINE." + "\r\n");
			} else if (boost::iequals(x[1], "LIST")) {
				vector <string> ip;
				vector <string> who;
				vector <string> motivo;
				string sql = "SELECT IP FROM GLINE;";
				ip = DB::SQLiteReturnVector(sql);
				sql = "SELECT NICK FROM GLINE;";
				who = DB::SQLiteReturnVector(sql);
				sql = "SELECT MOTIVO FROM GLINE;";
				motivo = DB::SQLiteReturnVector(sql);
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
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :Necesito mas datos. [ /operserv kill nick ]" + "\r\n");
			return;
		} else if (u->GetReg() == false) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :No te has registrado." + "\r\n");
			return;
		} else if (User::GetUserByNick(x[1]) == NULL) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :El nick no esta conectado." + "\r\n");
			return;
		}
		User *us = User::GetUserByNick(x[1]);
		for (auto it = us->channels.begin(); it != us->channels.end();) {
			Chan::PropagarQUIT(us, (*it)->GetNombre());
			Chan::Part(us, (*it)->GetNombre());
			it = us->channels.erase(it);
		}
			
		for (auto it = users.begin(); it != users.end(); it++)
			if (boost::iequals((*it)->GetID(), us->GetID())) {
				(*it)->GetSocket()->Close();
				users.erase(it);
				Servidor::SendToAllServers("QUIT " + us->GetID());
				break;
			}
	} else if (cmd == "DROP") {
		if (x.size() < 2) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :Necesito mas datos. [ /operserv drop (nick|canal) ]" + "\r\n");
			return;
		} else if (u->GetReg() == false) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :No te has registrado." + "\r\n");
			return;
		} else if (NickServ::IsRegistered(x[1]) == 0 && ChanServ::IsRegistered(x[1]) == 0) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :El nick no esta registrado." + "\r\n");
			return;
		} else if (Servidor::HUBExiste() == 0) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + "\r\n");
			return;
		} else if (u->Tiene_Modo('r') == false) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :No te has identificado, para hacer DROP necesitas tener el nick puesto." + "\r\n");
			return;
		} else if (NickServ::IsRegistered(x[1]) == 1) {
			string sql = "DELETE FROM NICKS WHERE NICKNAME='" + x[1] + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :El nick " + x[1] + " no ha sido borrado.\r\n");
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::SendToAllServers(sql);
			sql = "DELETE FROM OPTIONS WHERE NICKNAME='" + x[1] + "' COLLATE NOCASE;";
			DB::SQLiteNoReturn(sql);
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::SendToAllServers(sql);
			sql = "DELETE FROM ACCESS WHERE USUARIO='" + x[1] + "' COLLATE NOCASE;";
			DB::SQLiteNoReturn(sql);
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::SendToAllServers(sql);
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :El nick " + x[1] + " ha sido borrado.\r\n");
			return;
		} else if (ChanServ::IsRegistered(x[1]) == 1) {
			string sql = "DELETE FROM CANALES WHERE NOMBRE='" + x[1] + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				s->Write(":CHaN!*@* NOTICE " + u->GetNick() + " :El canal " + x[1] + " no ha sido borrado.\r\n");
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::SendToAllServers(sql);
			sql = "DELETE FROM ACCESS WHERE CANAL='" + x[1] + "' COLLATE NOCASE;";
			DB::SQLiteNoReturn(sql);
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::SendToAllServers(sql);
			sql = "DELETE FROM AKICK WHERE CANAL='" + x[1] + "' COLLATE NOCASE;";
			DB::SQLiteNoReturn(sql);
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::SendToAllServers(sql);
			s->Write(":CHaN!*@* NOTICE " + u->GetNick() + " :El canal " + x[1] + " ha sido borrado.\r\n");
			for (auto it = canales.begin(); it != canales.end(); it++)
				if (boost::iequals((*it)->GetNombre(), x[1])) {
					if ((*it)->Tiene_Modo('r') == true) {
						(*it)->Fijar_Modo('r', false);
						Chan::PropagarMODE("CHaN!*@*", "", x[1], 'r', 0, 1);
					}
				}
		}
	} else if (cmd == "SETPASS") {
		if (x.size() < 2) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :Necesito mas datos. [ /operserv setpass nick pass ]" + "\r\n");
			return;
		} else if (u->GetReg() == false) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :No te has registrado." + "\r\n");
			return;
		} else if (NickServ::IsRegistered(x[1]) == 0) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :El nick no esta registrado." + "\r\n");
			return;
		} else if (Servidor::HUBExiste() == 0) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + "\r\n");
			return;
		} else if (u->Tiene_Modo('r') == false) {
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :No te has identificado, para hacer DROP necesitas tener el nick puesto." + "\r\n");
			return;
		} else if (NickServ::IsRegistered(x[1]) == 1) {
			string sql = "UPDATE NICKS SET PASS='" + sha256(x[2]) + "' WHERE NICKNAME='" + x[1] + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :La pass del nick " + x[1] + " no ha podido ser cambiada.\r\n");
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::SendToAllServers(sql);
			s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :La pass del nick " + x[1] + " ha sido cambiada a " + x[2] + ".\r\n");
			return;
		}
	}
}

bool OperServ::IsGlined(string ip) {
	string sql = "SELECT IP from GLINE WHERE IP='" + ip + "' COLLATE NOCASE;";
	string retorno = DB::SQLiteReturnString(sql);
	if (boost::iequals(retorno, ip))
		return true;
	else
		return false;
}
