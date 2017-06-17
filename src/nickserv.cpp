#include "include.h"
#include "sha256.h"
#include <regex>

using namespace std;

NickServ *nickserv = new NickServ();

void NickServ::ProcesaMensaje(Socket *s, User *u, string mensaje) {
	if (mensaje.length() == 0 || mensaje == "\r\n" || mensaje == "\r" || mensaje == "\n" || mensaje == "||")
		return;
	vector<string> x;
	boost::split(x,mensaje,boost::is_any_of(" "));
	string cmd = x[0];
	mayuscula(cmd);
	
	if (cmd == "HELP") {
		s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :[ /nickserv register|drop|email|url|vhost|noaccess|nomemo|noop|showmail|onlyreg ]" + "\r\n");
		return;
	} else if (cmd == "REGISTER") {
		if (x.size() < 2) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :Necesito mas datos. [ /nickserv register password ]" + "\r\n");
			return;
		} else if (u->GetReg() == false) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :No te has registrado." + "\r\n");
			return;
		} else if (nickserv->IsRegistered(u->GetNick()) == 1) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :El nick ya esta registrado." + "\r\n");
			return;
		} else if (server->HUBExiste() == 0) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + "\r\n");
			return;
		} else {
			if (x[1].find(":") != std::string::npos || x[1].find("!") != std::string::npos) {
				s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :El password contiene caracteres no validos (!:)." + "\r\n");
				return;
			}
			string sql = "INSERT INTO NICKS VALUES ('" + u->GetNick() + "', '" + sha256(x[1]) + "', '', '', '',  " + boost::to_string(time(0)) + ", " + boost::to_string(time(0)) + ");";
			if (db->SQLiteNoReturn(sql) == false) {
				s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :El nick " + u->GetNick() + " no ha sido registrado.\r\n");
				return;
			}
			sql = "DB " + db->GenerateID() + " " + sql;
			db->AlmacenaDB(sql);
			server->SendToAllServers(sql + "||");
			sql = "INSERT INTO OPTIONS VALUES ('" + u->GetNick() + "', 0, 0, 0, 0, 0);";
			db->SQLiteNoReturn(sql);
			sql = "DB " + db->GenerateID() + " " + sql;
			db->AlmacenaDB(sql);
			server->SendToAllServers(sql + "||");
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :El nick " + u->GetNick() + " ha sido registrado.\r\n");
			if (u->Tiene_Modo('r') == false) {
				s->Write(":" + config->Getvalue("serverName") + " MODE " + u->GetNick() + " +r\r\n");
				u->Fijar_Modo('r', true);
			}
			return;
		}
	} else if (cmd == "DROP") {
		if (x.size() < 2) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :Necesito mas datos. [ /nickserv drop ]" + "\r\n");
			return;
		} else if (u->GetReg() == false) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :No te has registrado." + "\r\n");
			return;
		} else if (nickserv->IsRegistered(u->GetNick()) == 0) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :El nick no esta registrado." + "\r\n");
			return;
		} else if (server->HUBExiste() == 0) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + "\r\n");
			return;
		} else if (u->Tiene_Modo('r') == false) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :No te has identificado, para hacer DROP necesitas tener el nick puesto." + "\r\n");
			return;
		} else if (nickserv->Login(u->GetNick(), x[1]) == 0) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :La password no coincide." + "\r\n");
			return;
		} else {
			string sql = "DELETE FROM NICKS WHERE NICKNAME='" + u->GetNick() + "' COLLATE NOCASE;";
			if (db->SQLiteNoReturn(sql) == false) {
				s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :El nick " + u->GetNick() + " no ha sido borrado.\r\n");
				return;
			}
			sql = "DB " + db->GenerateID() + " " + sql;
			db->AlmacenaDB(sql);
			server->SendToAllServers(sql + "||");
			sql = "DELETE FROM OPTIONS WHERE NICKNAME='" + u->GetNick() + "' COLLATE NOCASE;";
			db->SQLiteNoReturn(sql);
			sql = "DB " + db->GenerateID() + " " + sql;
			db->AlmacenaDB(sql);
			server->SendToAllServers(sql + "||");
			sql = "DELETE FROM ACCESS WHERE USUARIO='" + u->GetNick() + "' COLLATE NOCASE;";
			db->SQLiteNoReturn(sql);
			sql = "DB " + db->GenerateID() + " " + sql;
			db->AlmacenaDB(sql);
			server->SendToAllServers(sql + "||");
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :El nick " + u->GetNick() + " ha sido borrado.\r\n");
			if (u->Tiene_Modo('r') == true) {
				s->Write(":" + config->Getvalue("serverName") + " MODE " + u->GetNick() + " -r\r\n");
				u->Fijar_Modo('r', false);
			}
			return;
		}
	} else if (cmd == "EMAIL") {
		if (x.size() < 2) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :Necesito mas datos. [ /nickserv email tu@email.com|off ]" + "\r\n");
			return;
		} else if (u->GetReg() == false) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :No te has registrado." + "\r\n");
			return;
		} else if (nickserv->IsRegistered(u->GetNick()) == 0) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :El nick no esta registrado." + "\r\n");
			return;
		} else if (server->HUBExiste() == 0) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + "\r\n");
			return;
		} else if (u->Tiene_Modo('r') == false) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :No te has identificado, para hacer EMAIL necesitas tener el nick puesto." + "\r\n");
			return;
		} else {
			string email;
			if (boost::iequals(x[1], "OFF", loc)) {
				email = "";
			} else {
				email = x[1];
			}
			if (email.find(";") != std::string::npos || email.find("'") != std::string::npos || email.find("\"") != std::string::npos) {
				s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :El email contiene caracteres no validos." + "\r\n");
				return;
			}
			string sql = "UPDATE NICKS SET EMAIL='" + email + "' WHERE NICKNAME='" + u->GetNick() + "' COLLATE NOCASE;";
			if (db->SQLiteNoReturn(sql) == false) {
				s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :El nick " + u->GetNick() + " no ha podido cambiar el correo electronico.\r\n");
				return;
			}
			sql = "DB " + db->GenerateID() + " " + sql;
			db->AlmacenaDB(sql);
			server->SendToAllServers(sql + "||");
			if (email.length() > 0)
				s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :Has cambiado tu EMAIL.\r\n");
			else
				s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :Has borrado tu EMAIL.\r\n");
			return;
		}
	} else if (cmd == "URL") {
		if (x.size() < 2) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :Necesito mas datos. [ /nickserv url www.tuweb.com|off ]" + "\r\n");
			return;
		} else if (u->GetReg() == false) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :No te has registrado." + "\r\n");
			return;
		} else if (nickserv->IsRegistered(u->GetNick()) == 0) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :El nick no esta registrado." + "\r\n");
			return;
		} else if (server->HUBExiste() == 0) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + "\r\n");
			return;
		} else if (u->Tiene_Modo('r') == false) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :No te has identificado, para hacer URL necesitas tener el nick puesto." + "\r\n");
			return;
		} else {
			string url;
			if (boost::iequals(x[1], "OFF", loc))
				url = "";
			else
				url = x[1];
			if (url.find(";") != std::string::npos || url.find("'") != std::string::npos || url.find("\"") != std::string::npos) {
				s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :El url contiene caracteres no validos." + "\r\n");
				return;
			}
			string sql = "UPDATE NICKS SET URL='" + url + "' WHERE NICKNAME='" + u->GetNick() + "' COLLATE NOCASE;";
			if (db->SQLiteNoReturn(sql) == false) {
				s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :El nick " + u->GetNick() + " no ha podido cambiar la web.\r\n");
				return;
			}
			sql = "DB " + db->GenerateID() + " " + sql;
			db->AlmacenaDB(sql);
			server->SendToAllServers(sql + "||");
			if (url.length() > 0)
				s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :Has cambiado tu URL.\r\n");
			else
				s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :Has borrado tu URL.\r\n");
			return;
		}
	} else if (cmd == "VHOST") {
		if (x.size() < 2) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :Necesito mas datos. [ /nickserv email tu.vhost|off ]" + "\r\n");
			return;
		} else if (u->GetReg() == false) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :No te has registrado." + "\r\n");
			return;
		} else if (nickserv->IsRegistered(u->GetNick()) == 0) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :El nick no esta registrado." + "\r\n");
			return;
		} else if (server->HUBExiste() == 0) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + "\r\n");
			return;
		} else if (u->Tiene_Modo('r') == false) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :No te has identificado, para hacer URL necesitas tener el nick puesto." + "\r\n");
			return;
		} else {
			string vHost;
			if (boost::iequals(x[1], "OFF", loc))
				vHost = "";
			else
				vHost = x[1];
			if (vHost.find(";") != std::string::npos || vHost.find("'") != std::string::npos || vHost.find("\"") != std::string::npos) {
				s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :El vHost contiene caracteres no validos." + "\r\n");
				return;
			}
			string sql = "UPDATE NICKS SET VHOST='" + vHost + "' WHERE NICKNAME='" + u->GetNick() + "' COLLATE NOCASE;";
			if (db->SQLiteNoReturn(sql) == false) {
				s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :El nick " + u->GetNick() + " no ha podido cambiar la web.\r\n");
				return;
			}
			sql = "DB " + db->GenerateID() + " " + sql;
			db->AlmacenaDB(sql);
			server->SendToAllServers(sql + "||");
			if (vHost.length() > 0)
				s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :Has cambiado tu VHOST.\r\n");
			else
				s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :Has borrado tu VHOST.\r\n");
			return;
		}
	} else if (cmd == "NOACCESS" || cmd == "SHOWMAIL" || cmd == "NOMEMO" || cmd == "NOOP" || cmd == "ONLYREG") {
		if (x.size() < 2) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :Necesito mas datos. [ /nickserv noaccess|showmail|nomemo|noop|onlyreg on|off ]" + "\r\n");
			return;
		} else if (u->GetReg() == false) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :No te has registrado." + "\r\n");
			return;
		} else if (nickserv->IsRegistered(u->GetNick()) == 0) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :El nick no esta registrado." + "\r\n");
			return;
		} else if (server->HUBExiste() == 0) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + "\r\n");
			return;
		} else if (u->Tiene_Modo('r') == false) {
			s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :No te has identificado, para hacer URL necesitas tener el nick puesto." + "\r\n");
			return;
		} else {
			int option = 0;
			if (boost::iequals(x[1], "OFF", loc))
				option = 0;
			else if (boost::iequals(x[1], "ON", loc))
				option = 1;
			else
				return;
			string sql = "UPDATE OPTIONS SET " + cmd + "=" + boost::to_string(option) + " WHERE NICKNAME='" + u->GetNick() + "' COLLATE NOCASE;";
			if (db->SQLiteNoReturn(sql) == false) {
				s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :El nick " + u->GetNick() + " no ha podido cambiar las opciones.\r\n");
				return;
			}
			sql = "DB " + db->GenerateID() + " " + sql;
			db->AlmacenaDB(sql);
			server->SendToAllServers(sql + "||");
			if (option == 1)
				s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :Has activado la opcion " + cmd + ".\r\n");
			else
				s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :Has desactivado la opcion " + cmd + ".\r\n");
			return;
		}
	}
	return;
}

void NickServ::UpdateLogin (User *u) {
	if (server->HUBExiste() == 0)
		return;

	string sql = "UPDATE NICKS SET LASTUSED=" + boost::to_string(time(0)) + " WHERE NICKNAME='" + u->GetNick() + "' COLLATE NOCASE;";
	if (db->SQLiteNoReturn(sql) == false) {
		oper->GlobOPs("Fallo al actualizar un nick.\r\n");
		return;
	}
	sql = "DB " + db->GenerateID() + " " + sql;
	db->AlmacenaDB(sql);
	server->SendToAllServers(sql + "||");
	return;
}

bool NickServ::IsRegistered(string nickname) {
	string sql = "SELECT NICKNAME from NICKS WHERE NICKNAME='" + nickname + "' COLLATE NOCASE;";
	string retorno = db->SQLiteReturnString(sql);
	if (boost::iequals(retorno, nickname, loc))
		return true;
	else
		return false;
} 

bool NickServ::Login (string nickname, string pass) {
	string sql = "SELECT PASS from NICKS WHERE NICKNAME='" + nickname + "' COLLATE NOCASE;";
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

bool NickServ::GetOption(string option, string nickname) {
	if (nickserv->IsRegistered(nickname) == 0)
		return false;
	string sql = "SELECT " + option + " FROM OPTIONS WHERE NICKNAME='" + nickname + "' COLLATE NOCASE;";
	return db->SQLiteReturnInt(sql);
}

string NickServ::GetvHost (string nickname) {
	if (nickserv->IsRegistered(nickname) == 0)
		return false;
	string sql = "SELECT VHOST FROM NICKS WHERE NICKNAME='" + nickname + "' COLLATE NOCASE;";
	return db->SQLiteReturnString(sql);
}
