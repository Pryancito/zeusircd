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

void Servidor::SQUIT(Servidor *s) {
	vector <string> servers;
	for (Servidor *srv = servidores.first(); srv != NULL; srv = servidores.next(srv)) {
		if (server->GetSocket(srv->GetNombre()) == server->GetSocket(s->GetNombre())) {
			servers.push_back(srv->GetNombre());
			for (unsigned int i = 0; i < srv->connected.size(); i++) {
				servers.push_back(srv->connected[i]);
			}
		}
	}
	for (unsigned int i = 0; i < servers.size(); i++)
		server->SQUIT(servers[i]);
}

void Servidor::SQUIT(Socket *s) {
	vector <string> servers;
	for (Servidor *srv = servidores.first(); srv != NULL; srv = servidores.next(srv)) {
		if (srv->GetSocket(srv->GetNombre()) == s) {
			servers.push_back(srv->GetNombre());
			for (unsigned int i = 0; i < srv->connected.size(); i++) {
				servers.push_back(srv->connected[i]);
			}
		}
	}
	for (unsigned int i = 0; i < servers.size(); i++)
		server->SQUIT(servers[i]);
}

void Servidor::SQUIT(string nombre) {
	vector <UserChan*> temp;
	vector <User *> usuar;
	for (Servidor *srv = servidores.first(); srv != NULL; srv = servidores.next(srv))
		if (boost::iequals(srv->GetNombre(), nombre, loc)) {
			for (User *us = users.first(); us != NULL; us = users.next(us)) {
				if (us->GetServer() == srv->GetNombre()) {
					Socket *sck = user->GetSocket(us->GetNick());
					for (UserChan *uc = usuarios.first(); uc != NULL; uc = usuarios.next(uc)) {
						if (boost::iequals(uc->GetID(), us->GetID(), loc)) {
							chan->PropagarQUIT(us, uc->GetNombre());
							temp.push_back(uc);
						}
					}
					usuar.push_back(us);

					if (user->EsMio(us->GetID()) == 1) {
						sck->Close();
						sock.del(sck);
					}
				}
			}
			server->SendToAllServers("SQUIT " + srv->GetID() + " " + config->Getvalue("serverID"));
			server->GetSocket(srv->GetNombre())->Close();
			servidores.del(srv);
		}
	for (unsigned int i = 0; i < temp.size(); i++) {
		UserChan *uc = temp[i];
		if (chan->GetUsers(uc->GetNombre()) == 0) {
			chan->DelChan(uc->GetNombre());
		}
		usuarios.del(uc);
	}
	for (unsigned int i = 0; i < usuar.size(); i++) {
		User *usr = usuar[i];
		if (user->EsMio(usr->GetID()) == 1)
			server->SendToAllServers("QUIT " + usr->GetID());
		users.del(usr);
	}
}

Socket *Servidor::GetSocket(string nombre) {
	for (Servidor *srv = servidores.first(); srv != NULL; srv = servidores.next(srv)) {
		if (boost::iequals(srv->GetNombre(), nombre, loc)) {
			return srv->socket;
		}
	}
	return NULL;
}

Servidor *Servidor::GetServer(string id) {
	for (Servidor *srv = servidores.first(); srv != NULL; srv = servidores.next(srv)) {
		if (boost::iequals(srv->GetID(), id, loc)) {
			return srv;
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
	s->Write("HUB " + config->Getvalue("hub") + "\n");
	
	string version = "VERSION ";
	if (db->GetLastRecord() != "") {
		version.append(db->GetLastRecord() + "\n");
	} else {
		version.append("0\n");
	}
	s->Write(version);
	
	for (Servidor *srv = servidores.first(); srv != NULL; srv = servidores.next(srv)) {
		string servidor = "SERVER " + srv->GetID() + " " + srv->GetNombre() + " " + srv->GetIP() + " 0";
		for (unsigned int i = 0; i < srv->connected.size(); i++) {
			servidor.append(" ");
			servidor.append(srv->connected[i]);
		}
		servidor.append("\n");
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
		s->Write("SNICK " + usr->GetID() + " " + usr->GetNick() + " " + usr->GetIdent() + " " + usr->GetIP() + " " + usr->GetCloakIP() + " " + boost::to_string(usr->GetLogin()) + " " + usr->GetServer() + " " + modos + "\n");
	}
	for (UserChan *uc = usuarios.first(); uc != NULL; uc = usuarios.next(uc)) {
		s->Write("SJOIN " + uc->GetID() + " " + uc->GetNombre() + " " + uc->GetModo() +  "\n");
	}
		
	for (BanChan *b = bans.first(); b != NULL; b = bans.next(b))
		s->Write("SBAN " + b->GetNombre() + " " + b->GetMask() + " " + b->GetWho() + " " + boost::to_string(b->GetTime()) + "\n");
		
	for (Memo *memo = memos.first(); memo != NULL; memo = memos.next(memo))
		s->Write("MEMO " + memo->sender + " " + memo->receptor + " " + boost::to_string(memo->time) + " " + memo->mensaje + "\n");

	operserv->ApplyGlines();
	return;
}

void Servidor::ListServers (Socket *s) {
	for (Servidor *srv = servidores.first(); srv != NULL; srv = servidores.next(srv))
		s->Write(":" + config->Getvalue("serverName") + " 002 :ID: " + srv->GetID() + " NOMBRE: " + srv->GetNombre() + " ( " + srv->GetIP() + " )" + "\r\n");
}

void Servidor::ProcesaMensaje (Socket *s, string mensaje) {
	if (mensaje.length() == 0 || mensaje == "\r\n" || mensaje == "\r" || mensaje == "\n")
		return;
	vector<string> x;
	boost::split(x,mensaje,boost::is_any_of(" "));
	string cmd = x[0];
	mayuscula(cmd);
	std::cout << mensaje << std::endl;
	if (cmd == "HUB") {
		if (x.size() < 2) {
			oper->GlobOPs("No hay HUB, cerrando conexion.");
			s->Quit();
			return;
		} else if (!boost::iequals(x[1], config->Getvalue("hub"), loc)) {
			oper->GlobOPs("Cerrando conexion. Los HUB no coinciden. ( " + config->Getvalue("hub") + " > " + x[1] + " )");
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
			for (unsigned int i = 6; i <= x.size(); i++)
				xs->AddLeaf(x[i]);
			servidores.add(xs);
			server->SendToAllButOne(s, mensaje);
			return;
		}
	} else if (cmd == "SNICK") {
		if (x.size() < 9) {
			oper->GlobOPs("SNICK Erroneo.");
			return;
		} else if (user->FindNick(x[2]) == 1) {
			User *u = new User(NULL, x[1]);
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
			server->SendToAllButOne(s, mensaje);
			string nickname = "ZeusiRCd-" + boost::to_string(rand() % 999999);
			Socket *sck = user->GetSocketByID(x[1]);
			if (sck != NULL)
				sck->Write(":" + u->GetNick() + " NICK " + nickname + "\r\n");
			server->SendToAllServers("SVSNICK " + u->GetID() + " " + nickname);
			chan->PropagarNICK(u, nickname);
			u->SetNick(nickname);
			users.add(u);
			return;
		} else if (user->FindNick(x[2]) == 0) {
			User *u = new User(NULL, x[1]);
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
			server->SendToAllButOne(s, mensaje);
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
				server->SendToAllButOne(s, mensaje);
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
			Socket *sck = user->GetSocketByID(x[1]);
			User *u = user->GetUser(x[1]);
			if (sck != NULL)
				sck->Write(":" + u->GetNick() + " NICK " + x[2] + "\r\n");
			chan->PropagarNICK(u, x[2]);
			u->SetNick(x[2]);
			SendToAllButOne(s, mensaje);
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
			SendToAllButOne(s, mensaje);
			return;
		}
	} else if (cmd == "QUIT") {
		if (x.size() < 2) {
			oper->GlobOPs("QUIT Erroneo.");
			return;
		} else if (user->FindNick(user->GetNickByID(x[1])) == 0)
			return;
		else {
			User *us = user->GetUser(x[1]);
			Socket *sck = user->GetSocketByID(x[1]);
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
					break;
				}
			if (sck != NULL)
				for (Socket *socket = sock.first(); socket != NULL; socket = sock.next(socket))
					if (boost::iequals(socket->GetID(), sck->GetID(), loc)) {
						socket->Close();
						sock.del(socket);
						break;
					}
			SendToAllButOne(s, mensaje);
		}
	} else if (cmd == "NOTICE" || cmd == "PRIVMSG") {
		int len = cmd.length() + x[1].length() + 1;
		string tmp = mensaje.substr(len);
		User *usr = user->GetUser(x[1]);
		if (x[2][0] == '#') {
			chan->PropagarMSG(usr, x[2], cmd + tmp + "\r\n");
			SendToAllButOne(s, mensaje);
		} else {
			Socket *sock = user->GetSocket(x[2]);
			User *ust = user->GetUserByNick(x[2]);
			std::cout << ":" + usr->FullNick() + " " + cmd + tmp + "\r\n" << std::endl;
			if (user->EsMio(ust->GetID()) == 1)
				sock->Write(":" + usr->FullNick() + " " + cmd + tmp + "\r\n");
			else
				SendToAllButOne(s, mensaje);
		}
		return;
	} else if (cmd == "SJOIN") {
		if (x.size() < 3) {
			oper->GlobOPs("SJOIN Erroneo.");
			return;
		} else {
			User *usr = user->GetUser(x[1]);
			chan->Join(usr, x[2]);
			chan->PropagarJOIN(usr, x[2]);
			if (x[3] != "x")
				chan->PropagarMODE("CHaN!*@*", user->GetNickByID(x[1]), x[2], x[3][0], 1, 0);
			SendToAllButOne(s, mensaje);
			return;
		}
	} else if (cmd == "SPART") {
		if (x.size() < 3) {
			oper->GlobOPs("SPART Erroneo.");
			return;
		} else {
			User *usr = user->GetUser(x[1]);
			chan->PropagarPART(usr, x[2]);
			chan->Part(usr, x[2]);
			SendToAllButOne(s, mensaje);
			return;
		}
	} else if (cmd == "DB") {
		string sql = mensaje.substr(20);
		db->SQLiteNoReturn(sql);
		db->AlmacenaDB(mensaje);
		server->SendToAllButOne(s, mensaje);
		return;
	} else if (cmd == "SQUIT") {
		if (x.size() == 2) {
			if (x[2] == config->Getvalue("serverID")) {
				Servidor *srv = server->GetServer(x[1]);
				server->SQUIT(srv);
				server->SendToAllButOne(s, mensaje);
			} else if (x[1] == config->Getvalue("serverID")) {
				Servidor *srv = server->GetServer(x[2]);
				server->SQUIT(srv);
				server->SendToAllButOne(s, mensaje);
			} else if (x[1] == x[2]){
				Servidor *srv = server->GetServer(x[1]);
				server->SQUIT(srv);
				server->SendToAllButOne(s, mensaje);
			}
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

			chan->PropagarMODE(x[1], x[4], x[2], x[3][1], action, 0);
			server->SendToAllButOne(s, mensaje);
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
			server->SendToAllButOne(s, mensaje);
		}
	} else if (cmd == "MEMODEL") {
		if (x.size() != 2) {
			oper->GlobOPs("MEMODEL Erroneo.");
			return;
		} else {
			for (Memo *memo = memos.first(); memo != NULL; memo = memos.next(memo))
				if (boost::iequals(x[1], memo->receptor, loc))
					memos.del(memo);
			server->SendToAllButOne(s, mensaje);
		}
	}
	return;
}

void Servidor::SendToAllServers (const string std) {
	for (Servidor *srv = servidores.first(); srv != NULL; srv = servidores.next(srv)) {
		Socket *sock = server->GetSocket(srv->GetNombre());
		if (sock != NULL && srv->GetID() != config->Getvalue("serverID"))
			sock->Write(std + '\n');
	}
}

void Servidor::SendToAllButOne (Socket *s, const string std) {
	for (Servidor *srv = servidores.first(); srv != NULL; srv = servidores.next(srv)) {
		Socket *sock = server->GetSocket(srv->GetNombre());
		if (sock != NULL && sock->GetID() != s->GetID() && srv->GetID() != config->Getvalue("serverID"))
			sock->Write(std + '\n');
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
