#include "include.h"
#include "sha256.h"
#include "../src/lista.cpp"
#include "../src/nodes.cpp"
#include <vector>

using namespace std;

List<Servidor*> servidores;
Servidor *server = new Servidor();

string Servidor::GetIP () {
	return ip;
}

std::mutex serv_mtx;

void Servidor::SQUIT(Socket *s) {
	vector <string> servers;
	for (Servidor *srv = servidores.first(); srv != NULL; srv = servidores.next(srv)) {
		if (srv->GetSocket(srv->GetNombre()) == s) {
			servers.push_back(srv->GetNombre());
			for (unsigned int i = 0; i < srv->connected.size(); i++) {
				servers.push_back(srv->connected[i]);
				for (User *usr = users.first(); usr != NULL; usr = users.next(usr)) {
					if (boost::iequals(usr->GetServer(), srv->connected[i], loc) || boost::iequals(usr->GetServer(), srv->GetNombre(), loc)) {
						for (UserChan *uc = usuarios.first(); uc != NULL; uc = usuarios.next(uc))
							if (boost::iequals(uc->GetID(), usr->GetID(), loc)) {
								chan->PropagarQUIT(usr, uc->GetNombre());
								chan->Part(usr, uc->GetNombre());
							}
						users.del(usr);
					}
				}
			}
			srv->GetSocket(srv->GetNombre())->Quit();
		}
	}
	for (Servidor *srv = servidores.first(); srv != NULL; srv = servidores.next(srv))
		for (unsigned int i = 0; i < servers.size(); i++)
			if (boost::iequals(srv->GetNombre(), servers[i], loc))
				servidores.del(srv);
				
}

void Servidor::SQUIT(string id) {
	vector <string> servers;
	for (Servidor *srv = servidores.first(); srv != NULL; srv = servidores.next(srv)) {
		if (boost::iequals(srv->GetID(), id, loc)) {
			servers.push_back(srv->GetNombre());
			for (unsigned int i = 0; i < srv->connected.size(); i++) {
				servers.push_back(srv->connected[i]);
				for (User *usr = users.first(); usr != NULL; usr = users.next(usr)) {
					if (boost::iequals(usr->GetServer(), srv->connected[i], loc) || boost::iequals(usr->GetServer(), srv->GetNombre(), loc)) {
						for (UserChan *uc = usuarios.first(); uc != NULL; uc = usuarios.next(uc))
							if (boost::iequals(uc->GetID(), usr->GetID(), loc)) {
								chan->PropagarQUIT(usr, uc->GetNombre());
								chan->Part(usr, uc->GetNombre());
							}
						users.del(usr);
					}
				}
			}
			srv->GetSocket(srv->GetNombre())->Quit();
		}
	}
	for (Servidor *srv = servidores.first(); srv != NULL; srv = servidores.next(srv))
		for (unsigned int i = 0; i < servers.size(); i++)
			if (boost::iequals(srv->GetNombre(), servers[i], loc))
				servidores.del(srv);
				
}

Socket *Servidor::GetSocket(string nombre) {
	for (Servidor *srv = servidores.first(); srv != NULL; srv = servidores.next(srv)) {
		if (boost::iequals(srv->GetNombre(), nombre, loc)) {
			return srv->socket;
		}
	}
	return NULL;
}

bool Servidor::Existe(string id) {
	for (Servidor *srv = servidores.first(); srv != NULL; srv = servidores.next(srv))
		if (boost::iequals(srv->GetID(), id, loc))
			return true;
	return false;
}

bool Servidor::IsAServer (string ip) {
	for (unsigned int i = 0; config->Getvalue("link["+boost::to_string(i)+"]ip").length() > 0; i++)
		if (config->Getvalue("link["+boost::to_string(i)+"]ip") == ip)
				return true;
	return false;
}

bool Servidor::IsConected (string ip) {
	for (Servidor *srv = servidores.first(); srv != NULL; srv = servidores.next(srv))
		if (boost::iequals(srv->GetIP(), ip, loc))
			return true;
	return false;
}

bool Servidor::HUBExiste() {
	for (Servidor *srv = servidores.first(); srv != NULL; srv = servidores.next(srv))
		if (boost::iequals(srv->GetNombre(), config->Getvalue("hub"), loc))
			return true;
	return false;
}

bool Servidor::SoyElHUB() {
	if (config->Getvalue("serverName") == config->Getvalue("hub"))
		return true;
	else
		return false;
}

void Servidor::SetNombre(string nombre_) {
	nombre = nombre_;
}

int Servidor::GetSaltos() {
	return saltos;
}

void Servidor::SetSaltos (int salt) {
	saltos = salt;
}

string Servidor::GetNombre() {
	return nombre;
}

void Servidor::SetIP (string ip_) {
	ip = ip_;
}

string Servidor::GetID () {
	return id;
}

void Servidor::AddLeaf(string nombre) {
	connected.push_back(nombre);
	return;
}

void Servidor::SendBurst (Socket *s) {
	string hub = "HUB " + config->Getvalue("hub") + "||";
	s->Write(hub);
	
	string version = "VERSION ";
	if (db->GetLastRecord() != "") {
		version.append(db->GetLastRecord());
	} else {
		version.append("0");
	}
		
	version.append("||");
	s->Write(version);
	
	for (Servidor *srv = servidores.first(); srv != NULL; srv = servidores.next(srv)) {
		string servidor = "SERVER " + srv->GetID() + " " + srv->GetNombre() + " " + srv->GetIP() + " " + boost::to_string(srv->GetSaltos());
		for (unsigned int i = 0; i < srv->connected.size(); i++) {
			servidor.append(" ");
			servidor.append(srv->connected[i]);
		}
		servidor.append("||");
		s->Write(servidor);
	}
	for (User *usr = users.first(); usr != NULL; usr = users.next(usr)) {
		string modos = "+";
		if (usr->Tiene_Modo('r') == true)
			modos.append("r");
		if (usr->Tiene_Modo('z') == true)
			modos.append("z");
		if (usr->Tiene_Modo('w') == true)
			modos.append("w");
		if (usr->Tiene_Modo('o') == true)
			modos.append("o");
		s->Write("SNICK " + usr->GetID() + " " + usr->GetNick() + " " + usr->GetIdent() + " " + usr->GetIP() + " " + usr->GetCloakIP() + " " + boost::to_string(usr->GetLogin()) + " " + usr->GetServer() + " " + modos + "||");
	}
	for (UserChan *uc = usuarios.first(); uc != NULL; uc = usuarios.next(uc))
		s->Write("SJOIN " + uc->GetNombre() + " " + uc->GetID() + " +" + uc->GetModo() + "||");
	for (BanChan *b = bans.first(); b != NULL; b = bans.next(b))
		s->Write("SBAN " + b->GetNombre() + " " + b->GetMask() + " " + b->GetWho() + " " + boost::to_string(b->GetTime()) + "||");
		
	for (Memo *memo = memos.first(); memo != NULL; memo = memos.next(memo))
		s->Write("MEMO " + memo->sender + " " + memo->receptor + " " + boost::to_string(memo->time) + " " + memo->mensaje + "||");

	return;
}

void Servidor::ListServers (Socket *s) {
	for (Servidor *srv = servidores.first(); srv != NULL; srv = servidores.next(srv))
		s->Write(":" + config->Getvalue("serverName") + " 002 :ID: " + srv->GetID() + " NOMBRE: " + srv->GetNombre() + " ( " + srv->GetIP() + " )" + "\r\n");
}

void Servidor::ProcesaMensaje (Socket *s, string mensaje) {
	if (mensaje.length() == 0 || mensaje == "\r\n" || mensaje == "\r" || mensaje == "\n" || mensaje == "||")
		return;
	vector<string> x;
	boost::split(x,mensaje,boost::is_any_of(" "));
	string cmd = x[0];
	mayuscula(cmd);
	
	if (cmd == "HUB") {
		if (x.size() < 2) {
			oper->GlobOPs("No hay HUB, cerrando conexion.");
			s->Quit();
			return;
		} else if (config->Getvalue("hub") != x[1]) {
			oper->GlobOPs("Cerrando conexion. Los HUB no coinciden.");
			s->Quit();
			return;
		}
	} else if (cmd == "VERSION") {
		if (x.size() < 2) {
			oper->GlobOPs("Error en las BDDs, cerrando conexion.");
			s->Quit();
			return;
		} else if (db->GetLastRecord() != x[1] && server->HUBExiste() == 1) {
				oper->GlobOPs("Sincronizando BDDs.");
				int syn = db->Sync(s, x[1]);
				oper->GlobOPs("BDDs sincronizadas, se actualizaron: " + boost::to_string(syn) + " registros.");
				return;
		}
	} else if (cmd == "SERVER") {
		if (x.size() < 5) {
			oper->GlobOPs("ERROR: SERVER invalido. Cerrando conexion.");
			s->Quit();
			return;
		} else if (server->Existe(x[1]) == 1) {
			oper->GlobOPs("ERROR: SERVER existente, colision de ID. Cerrando conexion.");
			s->Quit();
			return;
		} else {
			Socket *sk;
			if (x[4] == "0")
				sk = s;
			else
				sk = NULL;
			Servidor *xs = new Servidor(sk, x[1]);
				xs->SetNombre(x[2]);
				xs->SetIP(x[3]);
				xs->SetSaltos((int ) stoi(x[4])+1);

			for (unsigned int i = 5; i < x.size(); i++)
				xs->AddLeaf(x[i]);
			servidores.add(xs);
			server->SendToAllButOne(s, mensaje + "||");
			return;
		}
	} else if (cmd == "SNICK") {
		if (x.size() < 9) {
			oper->GlobOPs("SNICK Erroneo.");
			return;
		} else if (user->FindNick(x[2]) == 1 && user->EsMio(x[1]) == 1) {
			Socket *sock = user->GetSocketByID(x[1]);
			if (sock != NULL)
				sock->Quit();
			return;
		} else if (user->FindNick(x[2]) == 0) {
			User *u = new User(x[1]);
				u->SetNick(x[2]);
				u->SetIdent(x[3]);
				u->SetIP(x[4]);
				u->SetCloakIP(x[5]);
				u->SetLogin(stol(x[6]));
				u->SetNodo(x[7]);
				string modos = x[8];
				for (unsigned int i = 1; i < modos.length(); i++) {
					if (modos[i] == 'r')
						u->Fijar_Modo('r', true);
					else if (modos[i] == 'z')
						u->Fijar_Modo('z', true);
					else if (modos[i] == 'o')
						u->Fijar_Modo('o', true);
					else if (modos[i] == 'w')
						u->Fijar_Modo('w', true);
				}
			users.add(u);
			server->SendToAllButOne(s, mensaje + "||");
			return;
		} else {
			return;
		}
	} else if (cmd == "SKICK") {
		if (x.size() < 5) {
			oper->GlobOPs("SKICK Erroneo.");
			return;
		} else {
			User *usr = user->GetUser(x[1]);
			User *dest = user->GetUser(x[3]);
			if (usr != NULL && dest != NULL) {
				int posicion = 4 + x[0].length() + x[1].length() + x[2].length() + x[3].length();
				string motivo = mensaje.substr(posicion);
				chan->PropagarKICK(usr, x[2], dest, motivo);
				chan->Part(dest, x[2]);
				server->SendToAllButOne(s, mensaje + "||");
				return;
			}
		}
	} else if (cmd == "SVSNICK") {
		if (x.size() < 3) {
			oper->GlobOPs("SVSNICK Erroneo.");
			return;
		} else if (user->GetNickByID(x[1]) == "") {
			return;
		} else {
			User *usr = user->GetUser(user->GetNickByID(x[1]));
			if (usr != NULL) {
				chan->PropagarNICK(usr, x[2]);
				usr->SetNick(x[2]);
			}
			SendToAllButOne(s, mensaje + "||");
			return;
		}
	} else if (cmd == "SOPER") {
		if (x.size() < 2) {
			oper->GlobOPs("SOPER Erroneo.");
			return;
		} else {
			User *usr = user->GetUser(x[1]);
			if (usr != NULL) {
				usr->Fijar_Modo('o', true);
			}
			SendToAllButOne(s, mensaje + "||");
			return;
		}
	} else if (cmd == "QUIT") {
		if (x.size() < 2) {
			oper->GlobOPs("QUIT Erroneo.");
			return;
		} else {
			Socket *sock = user->GetSocketByID(x[1]);
			if (sock != NULL)
				sock->Quit();
			SendToAllButOne(s, mensaje + "||");
		}
	} else if (boost::iequals(x[1], "NOTICE", loc) || boost::iequals(x[1], "PRIVMSG", loc)) {
		string tmp = mensaje.substr(x[0].length()+1, mensaje.length());
		User *usr = user->GetUser(x[1]);
		if (x[2][0] == '#') {
			if (user == NULL)
				return;
			chan->PropagarMSG(usr, x[2], tmp);
			SendToAllButOne(s, mensaje + "||");
		} else {
			Socket *sock = user->GetSocketByID(x[1]);
			if (sock == NULL)
				SendToAllButOne(s, mensaje + "||");
			else
				sock->Write(":" + user->FullNick() + " " + tmp);
		}
		return;
	} else if (cmd == "SJOIN") {
		if (x.size() < 3) {
			oper->GlobOPs("SJOIN Erroneo.");
			return;
		} else {
			User *usr = user->GetUser(x[1]);
			if (usr != NULL) {
				chan->Join(usr, x[2]);
				chan->PropagarJOIN(usr, x[2]);
				chanserv->CheckModes(usr->GetNick(), x[2]);
			}
			SendToAllButOne(s, mensaje + "||");
			return;
		}
	} else if (cmd == "SPART") {
		if (x.size() < 3) {
			oper->GlobOPs("SPART Erroneo.");
			return;
		} else {
			User *usr = user->GetUser(x[1]);
			if (usr != NULL) {
				chan->PropagarPART(usr, x[2]);
				chan->Part(usr, x[2]);
			}
			SendToAllButOne(s, mensaje + "||");
			return;
		}
	} else if (cmd == "DB") {
		string sql = mensaje.substr(20);
		db->SQLiteNoReturn(sql);
		db->AlmacenaDB(mensaje);
		server->SendToAllButOne(s, mensaje + "||");
		return;
	} else if (cmd == "SQUIT") {
		if (x.size() == 2) {
			server->SQUIT(x[1]);
			server->SendToAllButOne(s, mensaje + "||");
			return;
		}
	} else if (cmd == "SMODE") {
		if (x.size() < 5) {
			oper->GlobOPs("SMODE Erroneo.");
			return;
		} else {
			bool action = 0;
			if (x[3][0] == '+')
				action = 1;
			else
				action = 0;
			if (x[3][1] == 'b')
				chan->ChannelBan(x[1], x[4], x[2]);
			
			chan->PropagarMODE(x[1], x[4], x[2], x[3][1], action);
			server->SendToAllButOne(s, mensaje + "||");
		}
	} else if (cmd == "MEMO") {
		if (x.size() < 5) {
			oper->GlobOPs("MEMO Erroneo.");
			return;
		} else {
			int pos = 8 + x[1].length() + x[2].length() + x[3].length();
			string msg = mensaje.substr(pos);
			for (Memo *memo = memos.first(); memo != NULL; memo = memos.next(memo))
				if (boost::iequals(x[1], memo->sender, loc) && boost::iequals(x[2], memo->receptor, loc) && boost::iequals(x[3], boost::to_string(memo->time), loc) && boost::iequals(msg, memo->mensaje, loc))
					return;
			Memo *memo = new Memo();
				memo->sender = x[1];
				memo->receptor = x[2];
				memo->time = stoi(x[3]);
				memo->mensaje = msg;
			memos.add(memo);
			server->SendToAllButOne(s, mensaje + "||");
		}
	} else if (cmd == "MEMODEL") {
		if (x.size() != 2) {
			oper->GlobOPs("MEMODEL Erroneo.");
			return;
		} else {
			for (Memo *memo = memos.first(); memo != NULL; memo = memos.next(memo))
				if (boost::iequals(x[1], memo->receptor, loc))
					memos.del(memo);
			server->SendToAllButOne(s, mensaje + "||");
		}
	}
	return;
}

void Servidor::SendToAllServers (const string std) {
	for (Servidor *srv = servidores.first(); srv != NULL; srv = servidores.next(srv)) {
		Socket *sock = srv->GetSocket(srv->GetNombre());
		if (sock != NULL)
			sock->Write(std + "||");
	}
}

void Servidor::SendToAllButOne (Socket *s, const string std) {
	for (Servidor *srv = servidores.first(); srv != NULL; srv = servidores.next(srv)) {
		Socket *sock = srv->GetSocket(srv->GetNombre());
		if (sock != NULL && sock != s)
			sock->Write(std + "||");
	}
}

bool Servidor::CheckClone(string ip) {
	int count = 1;
	for (User *usr = users.first(); usr != NULL; usr = users.next(usr))
		if (boost::iequals(usr->GetIP(), ip, loc))
			count++;
	if (count > stoi(config->Getvalue("clones")))
		return true;
	else
		return false;
}
