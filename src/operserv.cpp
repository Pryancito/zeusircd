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
	vector <string> ip;
	string sql = "SELECT IP FROM GLINE";
	ip = db->SQLiteReturnVector(sql);
	for (unsigned int i = 0; i < ip.size(); i++)
		insert_rule(ip[i]);
}

void OperServ::ProcesaMensaje(Socket *s, User *u, string mensaje) {
	if (mensaje.length() == 0 || mensaje == "\r\n" || mensaje == "\r" || mensaje == "\n" || mensaje == "||")
		return;
	vector<string> x;
	boost::split(x,mensaje,boost::is_any_of(" "));
	string cmd = x[0];
	mayuscula(cmd);
	
	if (cmd == "HELP") {
		s->Write(":OPeR!*@* NOTICE " + u->GetNick() + " :[ /operserv gline ]" + "\r\n");
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
				server->SendToAllServers(sql + "||");
				for (User *usr = users.first(); usr != NULL; usr = users.next(usr))
					if (boost::iequals(usr->GetIP(), x[2])) {
						Socket *sok = user->GetSocket(usr->GetNick());
						vector <UserChan*> temp;
						for (UserChan *uc = usuarios.first(); uc != NULL; uc = usuarios.next(uc)) {
							if (boost::iequals(uc->GetID(), usr->GetID(), loc)) {
								chan->PropagarQUIT(usr, uc->GetNombre());
								temp.push_back(uc);
								if (chan->IsEmpty(uc->GetNombre()) == 1) {
									chan->DelChan(uc->GetNombre());
								}
							}
						}
						for (unsigned int i = 0; i < temp.size(); i++)
							usuarios.del(temp[i]);

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
				server->SendToAllServers(sql + "||");
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
