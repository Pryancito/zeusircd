#include "include.h"

using namespace std;

void HostServ::ProcesaMensaje(Socket *s, User *u, string mensaje) {
	if (mensaje.length() == 0 || mensaje == "\r\n" || mensaje == "\r" || mensaje == "\n")
		return;
	vector<string> x;
	boost::split(x,mensaje,boost::is_any_of(" "));
	string cmd = x[0];
	mayuscula(cmd);
	
	if (cmd == "HELP") {
		s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :[ /hostserv register|drop|request|accept|off|list ]" + "\r\n");
		return;
	} else if (cmd == "REGISTER") {
		if (x.size() < 2) {
			s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :Necesito mas datos. [ /hostserv register path (owner) ]" + "\r\n");
			return;
		} else if (u->GetReg() == false) {
			s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :No te has registrado." + "\r\n");
			return;
		} else if (Servidor::HUBExiste() == 0) {
			s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + "\r\n");
			return;
		} else {
			string owner;
			if (x.size() == 2)
				owner = u->GetNick();
			else
				owner = x[2];
			if (checknick(owner) == false) {
				s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :El nick " + owner + " contiene caracteres no validos." + "\r\n");
				return;
			} else if (NickServ::IsRegistered(owner) == 0) {
				s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :El nick " + owner + " no esta registrado." + "\r\n");
				return;
			} else if (HostServ::CheckPath(x[1]) == false) {
				s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :El path " + x[1] + " no es valido." + "\r\n");
				return;
			} else if (HostServ::Owns(u, x[1]) == false && x[1].find("/") != std::string::npos) {
				s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :El path " + x[1] + " no te pertenece." + "\r\n");
				return;
			} else {
				string sql = "SELECT PATH from PATHS WHERE PATH='" + x[1] + "' COLLATE NOCASE;";
				if (boost::iequals(DB::SQLiteReturnString(sql), x[1]) == true) {
					s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :El path " + x[1] + " ya esta registrado.\r\n");
					return;
				}
				sql = "SELECT VHOST from NICKS WHERE VHOST='" + x[1] + "' COLLATE NOCASE;";
				if (boost::iequals(DB::SQLiteReturnString(sql), x[1]) == true) {
					s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :El path " + x[1] + " ya esta registrado.\r\n");
					return;
				}
				sql = "INSERT INTO PATHS VALUES ('" + owner + "', '" + x[1] + "');";
				if (DB::SQLiteNoReturn(sql) == false) {
					s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :El path " + x[1] + " no ha podido ser registrado.\r\n");
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::SendToAllServers(sql);
				s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :El path " + x[1] + " ha sido registrado bajo la cuenta " + owner + ".\r\n");
				return;
			}
		}
	} else if (cmd == "DROP") {
		if (x.size() < 2) {
			s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :Necesito mas datos. [ /hostserv drop path ]" + "\r\n");
			return;
		} else if (u->GetReg() == false) {
			s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :No te has registrado." + "\r\n");
			return;
		} else if (Servidor::HUBExiste() == 0) {
			s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + "\r\n");
			return;
		} else {
			if (HostServ::CheckPath(x[1]) == false) {
				s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :El path " + x[1] + " no es valido." + "\r\n");
				return;
			} else if (HostServ::Owns(u, x[1]) == false) {
				s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :El path " + x[1] + " no te pertenece." + "\r\n");
				return;
			} else {
				if (HostServ::DeletePath(x[1]) == true)
					s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :El path " + x[1] + " ha sido borrado." + "\r\n");
				else
					s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :El path " + x[1] + " no ha podido ser borrado." + "\r\n");
				return;
			}
		}
	} else if (cmd == "REQUEST") {
		if (x.size() < 2) {
			s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :Necesito mas datos. [ /hostserv request path|off ]" + "\r\n");
			return;
		} else if (u->GetReg() == false) {
			s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :No te has registrado." + "\r\n");
			return;
		} else if (Servidor::HUBExiste() == 0) {
			s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + "\r\n");
			return;
		} else {
			if (HostServ::CheckPath(x[1]) == false) {
				s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :El path " + x[1] + " no es valido." + "\r\n");
				return;
			} else if (HostServ::GotRequest(u->GetNick()) == true) {
				s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :Ya tienes una peticion de vHost." + "\r\n");
				return;
			} else if (HostServ::PathIsInvalid(x[1]) == true) {
				s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :El path " + x[1] + " no es valido." + "\r\n");
				return;
			} else if (boost::iequals(x[1], "OFF", loc)) {
				string sql = "DELETE FROM REQUEST WHERE OWNER='" + u->GetNick() + "' COLLATE NOCASE;";
				if (DB::SQLiteNoReturn(sql) == false) {
					s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :Tu peticion no ha podido ser borrada." + "\r\n");
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::SendToAllServers(sql);
				s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :Tu peticion ha sido borrada. " + "\r\n");
				return;
			} else {
				string sql = "INSERT INTO REQUEST VALUES ('" + u->GetNick() + "', '" + x[1] + "', " + boost::to_string(time(0)) + ");";
				if (DB::SQLiteNoReturn(sql) == false) {
					s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :Tu peticion no ha podido ser registrada.\r\n");
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::SendToAllServers(sql);
				s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :Tu peticion ha sido registrada con exito." + "\r\n");
				return;
			}
		}
	} else if (cmd == "ACCEPT") {
		if (x.size() < 2) {
			s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :Necesito mas datos. [ /hostserv accept nick ]" + "\r\n");
			return;
		} else if (u->GetReg() == false) {
			s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :No te has registrado." + "\r\n");
			return;
		} else if (Servidor::HUBExiste() == 0) {
			s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + "\r\n");
			return;
		} else {
			if (checknick(x[1]) == false) {
				s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :El nick " + x[1] + " contiene caracteres no validos." + "\r\n");
				return;
			} else if (NickServ::IsRegistered(x[1]) == false) {
				s->Write(":NiCK!*@* NOTICE " + u->GetNick() + " :El nick " + x[1] + " no esta registrado." + "\r\n");
				return;
			} else if (HostServ::GotRequest(x[1]) == false) {
				s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :El nick " + x[1] + " no tiene una peticion de vHost." + "\r\n");
				return;
			} else {
				string sql = "SELECT PATH from REQUEST WHERE OWNER='" + x[1] + "' COLLATE NOCASE;";
				string path = DB::SQLiteReturnString(sql);
				sql = "DELETE FROM REQUEST WHERE OWNER='" + x[1] + "' COLLATE NOCASE;";
				if (DB::SQLiteNoReturn(sql) == false) {
					s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :Tu aceptacion no ha podido ser finalizada." + "\r\n");
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::SendToAllServers(sql);
				sql = "UPDATE NICKS SET VHOST='" + path + "' WHERE NICKNAME='" + x[1] + "' COLLATE NOCASE;";
				if (DB::SQLiteNoReturn(sql) == false) {
					s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :Tu aceptacion no ha podido ser finalizada.\r\n");
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::SendToAllServers(sql);
				s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :Tu aceptacion ha sido finalizada con exito." + "\r\n");
				return;
			}
		}
	} else if (cmd == "OFF") {
		if (u->GetReg() == false) {
			s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :No te has registrado." + "\r\n");
			return;
		} else if (Servidor::HUBExiste() == 0) {
			s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + "\r\n");
			return;
		} else if (NickServ::IsRegistered(u->GetNick()) == false) {
			s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :Tu nick no esta registrado." + "\r\n");
			return;
		} else if (NickServ::GetvHost(u->GetNick()) == "") {
			s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :Tu nick no tiene un vHost Configurado." + "\r\n");
			return;
		} else {
			string sql = "UPDATE NICKS SET VHOST='' WHERE NICKNAME='" + u->GetNick() + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :Tu eliminacion de vHost no ha podido ser finalizada.\r\n");
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::SendToAllServers(sql);
			s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :Tu eliminacion de vHost ha sido finalizada con exito." + "\r\n");
			return;
		}
	} else if (cmd == "LIST") {
		if (u->GetReg() == false) {
			s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :No te has registrado." + "\r\n");
			return;
		} else if (Servidor::HUBExiste() == 0) {
			s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + "\r\n");
			return;
		} else if (NickServ::IsRegistered(u->GetNick()) == false) {
			s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :Tu nick no esta registrado." + "\r\n");
			return;
		} else {
			if (x.size() == 1) {
				vector <string> subpaths;
				vector <string> owner;
				string sql = "SELECT PATH from PATHS WHERE OWNER='" + u->GetNick() + "' COLLATE NOCASE;";
				vector <string> paths = DB::SQLiteReturnVector(sql);
				for (unsigned int i = 0; i < paths.size(); i++) {
					vector <string> temp1;
					vector <string> temp2;
					sql = "SELECT OWNER from REQUEST WHERE PATH LIKE '" + paths[i] + "%' COLLATE NOCASE;";
					temp1 = DB::SQLiteReturnVector(sql);
					sql = "SELECT PATH from REQUEST WHERE PATH LIKE '" + paths[i] + "%' COLLATE NOCASE;";
					temp2 = DB::SQLiteReturnVector(sql);
					for (unsigned int j = 0; j < temp1.size(); j++) {
						subpaths.push_back(temp2[j]);
						owner.push_back(temp1[j]);
					}
				}
				string temp1;
				for (unsigned int i = 0; i < subpaths.size(); i++) {
					if (temp1.find(owner[i]) == std::string::npos) {
						s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :<" + owner[i] + "> PATH: " + subpaths[i] + "\r\n");
						temp1.append(owner[i] + ' ');
					}
				}
				s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :Fin de la lista." + "\r\n");
				return;
			} else if (x.size() == 2) {
				string search = x[1];
				string sql = "SELECT PATH from PATHS;";
				vector <string> paths = DB::SQLiteReturnVector(sql);
				sql = "SELECT OWNER from PATHS;";
				vector <string> owner = DB::SQLiteReturnVector(sql);
				for (unsigned int i = 0; i < paths.size(); i++) {
					boost::algorithm::to_lower(paths[i]);
					boost::algorithm::to_lower(search);
					if (User::Match(search.c_str(), paths[i].c_str()) == 1)
						s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :<" + owner[i] + "> PATH: " + paths[i] + "\r\n");
				}
				s->Write(":vHost!*@* NOTICE " + u->GetNick() + " :Fin de la lista." + "\r\n");
				return;
			}
		}
	}
}

bool HostServ::CheckPath(string path) {
	vector<string> subpaths;
	boost::split(subpaths,path,boost::is_any_of("/"));
	if (subpaths.size() < 1 || subpaths.size() > 10)
		return false;
	for (unsigned int i = 0; i < subpaths.size(); i++) {
		if (subpaths[i].length() == 0)
			return false;
		else if (checknick(subpaths[i]) == false)
			return false;
	}
	return true;
}

bool HostServ::Owns(User *u, string path) {
	vector<string> subpaths;
	boost::split(subpaths,path,boost::is_any_of("/"));
	string pp = subpaths[0];
	for (unsigned int i = 1; i < subpaths.size(); i++) {
		string sql = "SELECT OWNER from PATHS WHERE PATH='" + pp + "' COLLATE NOCASE;";
		string retorno = DB::SQLiteReturnString(sql);
		if (boost::iequals(retorno, u->GetNick(), loc))
			return true;
		pp.append("/" + subpaths[i]);
	}
	return false;
}

bool HostServ::DeletePath(string path) {
	string sql = "SELECT PATH from PATHS WHERE PATH LIKE '" + path + "%' COLLATE NOCASE;";
	vector <string> retorno = DB::SQLiteReturnVector(sql);
	for (unsigned int i = 0; i < retorno.size(); i++) {
		string sql = "DELETE FROM PATHS WHERE PATH='" + retorno[i] + "' COLLATE NOCASE;";
		if (DB::SQLiteNoReturn(sql) == false) {
			return false;
		}
		sql = "DB " + DB::GenerateID() + " " + sql;
		DB::AlmacenaDB(sql);
		Servidor::SendToAllServers(sql);
	}
	sql = "SELECT NICKNAME from NICKS WHERE VHOST LIKE '" + path + "%' COLLATE NOCASE;";
	retorno = DB::SQLiteReturnVector(sql);
	for (unsigned int i = 0; i < retorno.size(); i++) {
		string sql = "UPDATE NICKS SET VHOST='' WHERE NICKNAME='" + retorno[i] + "' COLLATE NOCASE;";
		if (DB::SQLiteNoReturn(sql) == false) {
			return false;
		}
		sql = "DB " + DB::GenerateID() + " " + sql;
		DB::AlmacenaDB(sql);
		Servidor::SendToAllServers(sql);
	}
	return true;
}

bool HostServ::GotRequest (string user) {
	string sql = "SELECT OWNER from REQUEST WHERE OWNER='" + user + "' COLLATE NOCASE;";
	string retorno = DB::SQLiteReturnString(sql);
	if (boost::iequals(retorno, user, loc))
		return true;
	else
		return false;
}

bool HostServ::PathIsInvalid (string path) {
	string sql = "SELECT PATH from PATHS WHERE PATH='" + path + "' COLLATE NOCASE;";
	string retorno = DB::SQLiteReturnString(sql);
	if (boost::iequals(retorno, path, loc))
		return true;
	sql = "SELECT VHOST from NICKS WHERE VHOST='" + path + "' COLLATE NOCASE;";
	retorno = DB::SQLiteReturnString(sql);
	if (boost::iequals(retorno, path, loc))
		return true;
		
	return false;
}
