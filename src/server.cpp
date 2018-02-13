#include "include.h"
#include "sha256.h"
#include "../src/lista.cpp"
#include "../src/nodes.cpp"
#include <vector>

using namespace std;

List<Servidor*> servidores;
std::mutex servidores_mtx;

string Servidor::GetIP () {
	return ip;
}

void Servidor::SQUIT(Servidor *s) {
	vector <string> servers;
	for (Servidor *srv = servidores.first(); srv != NULL; srv = servidores.next(srv)) {
		if (Servidor::GetSocket(srv->GetNombre()) == Servidor::GetSocket(s->GetNombre())) {
			servers.push_back(srv->GetNombre());
			for (unsigned int i = 0; i < srv->connected.size(); i++) {
				servers.push_back(srv->connected[i]);
			}
		}
	}
	for (unsigned int i = 0; i < servers.size(); i++)
		Servidor::SQUIT(servers[i]);
}

void Servidor::SQUIT(Socket *s) {
	vector <string> servers;
	for (Servidor *srv = servidores.first(); srv != NULL; srv = servidores.next(srv)) {
		if (Servidor::GetSocket(srv->GetNombre()) == s) {
			servers.push_back(srv->GetNombre());
			for (unsigned int i = 0; i < srv->connected.size(); i++) {
				servers.push_back(srv->connected[i]);
			}
		}
	}
	for (unsigned int i = 0; i < servers.size(); i++)
		Servidor::SQUIT(servers[i]);
}

void Servidor::SQUIT(string nombre) {
	vector <UserChan*> temp;
	vector <User *> usuar;
	for (Servidor *srv = servidores.first(); srv != NULL; srv = servidores.next(srv))
		if (boost::iequals(srv->GetNombre(), nombre, loc)) {
			for (User *us = users.first(); us != NULL; us = users.next(us)) {
				if (us->GetServer() == srv->GetNombre()) {
					Socket *sck = User::GetSocket(us->GetNick());
					for (UserChan *uc = usuarios.first(); uc != NULL; uc = usuarios.next(uc)) {
						if (boost::iequals(uc->GetID(), us->GetID(), loc)) {
							Chan::PropagarQUIT(us, uc->GetNombre());
							temp.push_back(uc);
						}
					}
					usuar.push_back(us);

					if (User::EsMio(us->GetID()) == 1) {
						sck->Close();
						sock.del(sck);
					}
				}
			}
			Servidor::SendToAllServers("SQUIT " + srv->GetID() + " " + config->Getvalue("serverID"));
			Servidor::GetSocket(srv->GetNombre())->Close();
			servidores.del(srv);
		}
	for (unsigned int i = 0; i < temp.size(); i++) {
		UserChan *uc = temp[i];
		if (Chan::GetUsers(uc->GetNombre()) == 0) {
			Chan::DelChan(uc->GetNombre());
		}
		usuarios.del(uc);
	}
	for (unsigned int i = 0; i < usuar.size(); i++) {
		User *usr = usuar[i];
		if (User::EsMio(usr->GetID()) == 1)
			Servidor::SendToAllServers("QUIT " + usr->GetID());
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
	if (DB::GetLastRecord() != "") {
		version.append(DB::GetLastRecord() + "\n");
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
	for (BanChan *b = bans.first(); b != NULL; b = bans.next(b))
		s->Write("SBAN " + b->GetNombre() + " " + b->GetMask() + " " + b->GetWho() + " " + boost::to_string(b->GetTime()) + "\n");
	for (UserChan *uc = usuarios.first(); uc != NULL; uc = usuarios.next(uc)) {
		s->Write("SJOIN " + uc->GetID() + " " + uc->GetNombre() + " " + uc->GetModo() +  "\n");
	}
	for (Memo *memo = memos.first(); memo != NULL; memo = memos.next(memo))
		s->Write("MEMO " + memo->sender + " " + memo->receptor + " " + boost::to_string(memo->time) + " " + memo->mensaje + "\n");

	OperServ::ApplyGlines();
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
	if (cmd == "HUB") {
		if (x.size() < 2) {
			Oper::GlobOPs("No hay HUB, cerrando conexion.");
			s->Quit();
			return;
		} else if (!boost::iequals(x[1], config->Getvalue("hub"), loc)) {
			Oper::GlobOPs("Cerrando conexion. Los HUB no coinciden. ( " + config->Getvalue("hub") + " > " + x[1] + " )");
			s->Quit();
			return;
		}
	} else if (cmd == "VERSION") {
		if (x.size() < 2) {
			Oper::GlobOPs("Error en las BDDs, cerrando conexion.");
			s->Quit();
			return;
		} else if (DB::GetLastRecord() != x[1] && Servidor::HUBExiste() == 1) {
				Oper::GlobOPs("Sincronizando BDDs.");
				int syn = DB::Sync(s, x[1]);
				Oper::GlobOPs("BDDs sincronizadas, se actualizaron: " + boost::to_string(syn) + " registros.");
				return;
		}
	} else if (cmd == "SERVER") {
		if (x.size() < 5) {
			Oper::GlobOPs("ERROR: SERVER invalido. Cerrando conexion.");
			s->Quit();
			return;
		} else if (Servidor::Existe(x[1]) == 1) {
			Oper::GlobOPs("ERROR: SERVER existente, colision de ID. Cerrando conexion.");
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
			Servidor::SendToAllButOne(s, mensaje);
			return;
		}
	} else if (cmd == "SNICK") {
		if (x.size() < 9) {
			Oper::GlobOPs("SNICK Erroneo.");
			return;
		} else if (!User::GetNickByID(x[1]).empty()) {
			Servidor::SendToAllServers("QUIT " + x[1]);
			return;
		} else if (User::FindNick(x[2]) == 1) {
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
			Servidor::SendToAllButOne(s, mensaje);
			string nickname = "ZeusiRCd-" + boost::to_string(rand() % 999999);
			Socket *sck = User::GetSocketByID(x[1]);
			if (sck != NULL)
				sck->Write(":" + u->GetNick() + " NICK " + nickname + "\r\n");
			Servidor::SendToAllServers("SVSNICK " + u->GetID() + " " + nickname);
			Chan::PropagarNICK(u, nickname);
			u->SetNick(nickname);
			users.add(u);
			return;
		} else if (User::FindNick(x[2]) == 0) {
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
			Servidor::SendToAllButOne(s, mensaje);
			return;
		} else {
			return;
		}
	} else if (cmd == "SBAN") {
		if (x.size() < 5) {
			Oper::GlobOPs("SBAN Erroneo.");
			return;
		} else {
			for (BanChan *bc = bans.first(); bc != NULL; bc = bans.next(bc)) {
				if (boost::iequals(bc->GetNombre(), x[1], loc)) {
					std::string baneo = bc->GetMask();
					boost::algorithm::to_lower(baneo);
					if (User::Match(baneo.c_str(), x[2].c_str()) == 1) {
						return;
					}
				}
			}
			std::time_t time = (long ) atoi(x[4].c_str());
			BanChan *b = new BanChan(x[1], x[2], x[3], time);
			bans.add(b);
			Chan::PropagarMODE(x[3], x[2], x[1], 'b', true, 0);
			Servidor::SendToAllButOne(s, mensaje);
		}
	} else if (cmd == "SKICK") {
		if (x.size() < 5) {
			Oper::GlobOPs("SKICK Erroneo.");
			return;
		} else {
			User *usr = User::GetUser(x[1]);
			User *dest = User::GetUser(x[3]);
			if (usr != NULL && dest != NULL && Chan::IsInChan(dest, x[2]) == 1) {
				int posicion = 4 + x[0].length() + x[1].length() + x[2].length() + x[3].length();
				string motivo = mensaje.substr(posicion);
				Chan::PropagarKICK(usr, x[2], dest, motivo);
				Chan::Part(dest, x[2]);
				Servidor::SendToAllButOne(s, mensaje);
				return;
			}
		}
	} else if (cmd == "SVSNICK") {
		if (x.size() < 3) {
			Oper::GlobOPs("SVSNICK Erroneo.");
			return;
		} else if (User::GetNickByID(x[1]) == "") {
			return;
		} else {
			Socket *sck = User::GetSocketByID(x[1]);
			User *u = User::GetUser(x[1]);
			if (sck != NULL)
				sck->Write(":" + u->GetNick() + " NICK " + x[2] + "\r\n");
			Chan::PropagarNICK(u, x[2]);
			u->SetNick(x[2]);
			SendToAllButOne(s, mensaje);
			return;
		}
	} else if (cmd == "SOPER") {
		if (x.size() < 2) {
			Oper::GlobOPs("SOPER Erroneo.");
			return;
		} else {
			User *usr = User::GetUser(x[1]);
			if (usr != NULL) {
				usr->Fijar_Modo('o', true);
			}
			SendToAllButOne(s, mensaje);
			return;
		}
	} else if (cmd == "QUIT") {
		if (x.size() < 2) {
			Oper::GlobOPs("QUIT Erroneo.");
			return;
		} else if (User::FindNick(User::GetNickByID(x[1])) == 0)
			return;
		else {
			User *us = User::GetUser(x[1]);
			Socket *sck = User::GetSocketByID(x[1]);
			vector <UserChan*> temp;
			for (UserChan *uc = usuarios.first(); uc != NULL; uc = usuarios.next(uc)) {
				if (boost::iequals(uc->GetID(), us->GetID(), loc)) {
					temp.push_back(uc);
				}
			}
			for (unsigned int i = 0; i < temp.size(); i++) {
				UserChan *uc = temp[i];
				Chan::PropagarQUIT(us, uc->GetNombre());
				usuarios.del(uc);
				if (Chan::GetUsers(uc->GetNombre()) == 0) {
					Chan::DelChan(uc->GetNombre());
				}
			}
			for (User *usr = users.first(); usr != NULL; usr = users.next(usr))
				if (boost::iequals(usr->GetID(), us->GetID(), loc)) {
					users.del(usr);
					break;
				}
			if (sck != NULL) {
				for (Socket *socket = sock.first(); socket != NULL; socket = sock.next(socket))
					if (boost::iequals(socket->GetID(), sck->GetID(), loc)) {
						socket->Close();
						sock.del(socket);
						break;
					}
			}
			SendToAllButOne(s, mensaje);
		}
	} else if (cmd == "NOTICE" || cmd == "PRIVMSG") {
		int len = cmd.length() + x[1].length() + 1;
		string tmp = mensaje.substr(len);
		User *usr = User::GetUser(x[1]);
		if (x[2][0] == '#') {
			Chan::PropagarMSG(usr, x[2], cmd + tmp + "\r\n");
			SendToAllButOne(s, mensaje);
		} else {
			Socket *sock = User::GetSocket(x[2]);
			User *ust = User::GetUserByNick(x[2]);
			std::cout << ":" + usr->FullNick() + " " + cmd + tmp + "\r\n" << std::endl;
			if (User::EsMio(ust->GetID()) == 1)
				sock->Write(":" + usr->FullNick() + " " + cmd + tmp + "\r\n");
			else
				SendToAllButOne(s, mensaje);
		}
		return;
	} else if (cmd == "SJOIN") {
		if (x.size() < 3) {
			Oper::GlobOPs("SJOIN Erroneo.");
			return;
		} else {
			User *usr = User::GetUser(x[1]);
			string mascara = usr->GetNick() + "!" + usr->GetIdent() + "@" + usr->GetCloakIP();
			if (Chan::IsBanned(usr, x[2]) == 1) {
				Servidor::SendToAllServers("SKICK " + usr->GetID() + " " + x[2] + " " + usr->GetID() + " Estas baneado, no puedes entrar al canal." + "\r\n");
				return;
			}
			else if (ChanServ::IsAKICK(mascara, x[2]) == 1 && Oper::IsOper(usr) == 0) {
				Servidor::SendToAllServers("SKICK " + usr->GetID() + " " + x[2] + " " + usr->GetID() + " Tienes AKICK en este canal, no puedes entrar." + "\r\n");
				return;
			}
			else if (NickServ::IsRegistered(usr->GetNick()) == 1 && !NickServ::GetvHost(usr->GetNick()).empty()) {
				mascara = usr->GetNick() + "!" + usr->GetIdent() + "@" + NickServ::GetvHost(usr->GetNick());
				if (ChanServ::IsAKICK(mascara, x[2]) == 1 && Oper::IsOper(usr) == 0) {
					Servidor::SendToAllServers("SKICK " + usr->GetID() + " " + x[2] + " " + usr->GetID() + " Tienes AKICK en este canal, no puedes entrar." + "\r\n");
					return;
				}
			}
			Chan::Join(usr, x[2]);
			Chan::PropagarJOIN(usr, x[2]);
			if (x[3] != "x")
				Chan::PropagarMODE("CHaN!*@*", usr->GetNick(), x[2], x[3][0], 1, 0);
			SendToAllButOne(s, mensaje);
			return;
		}
	} else if (cmd == "SPART") {
		if (x.size() < 3) {
			Oper::GlobOPs("SPART Erroneo.");
			return;
		} else {
			User *usr = User::GetUser(x[1]);
			Chan::PropagarPART(usr, x[2]);
			Chan::Part(usr, x[2]);
			SendToAllButOne(s, mensaje);
			return;
		}
	} else if (cmd == "DB") {
		string sql = mensaje.substr(20);
		DB::SQLiteNoReturn(sql);
		DB::AlmacenaDB(mensaje);
		Servidor::SendToAllButOne(s, mensaje);
		return;
	} else if (cmd == "SQUIT") {
		if (x.size() == 3) {
			if (x[2] == config->Getvalue("serverID")) {
				Servidor *srv = Servidor::GetServer(x[1]);
				Servidor::SQUIT(srv);
				Servidor::SendToAllButOne(s, mensaje);
			} else if (x[1] == config->Getvalue("serverID")) {
				Servidor *srv = Servidor::GetServer(x[2]);
				Servidor::SQUIT(srv);
				Servidor::SendToAllButOne(s, mensaje);
			} else if (x[1] == x[2]) {
				Servidor *srv = Servidor::GetServer(x[1]);
				Servidor::SQUIT(srv);
				Servidor::SendToAllButOne(s, mensaje);
			} else {
				Servidor *srv = Servidor::GetServer(x[1]);
				Servidor::SQUIT(srv);
				Servidor::SendToAllButOne(s, mensaje);
			}
			return;
		}
	} else if (cmd == "SMODE") {
		if (x.size() < 5) {
			Oper::GlobOPs("SMODE Erroneo.");
			return;
		} else {
			bool action = 0;
			if (x[3][0] == '+')
				action = 1;
			else
				action = 0;
			if (x[3][1] == 'b')
				Chan::ChannelBan(x[1], x[4], x[2]);

			Chan::PropagarMODE(x[1], x[4], x[2], x[3][1], action, 0);
			Servidor::SendToAllButOne(s, mensaje);
		}
	} else if (cmd == "MEMO") {
		if (x.size() < 5) {
			Oper::GlobOPs("MEMO Erroneo.");
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
			Servidor::SendToAllButOne(s, mensaje);
		}
	} else if (cmd == "MEMODEL") {
		if (x.size() != 2) {
			Oper::GlobOPs("MEMODEL Erroneo.");
			return;
		} else {
			for (Memo *memo = memos.first(); memo != NULL; memo = memos.next(memo))
				if (boost::iequals(x[1], memo->receptor, loc))
					memos.del(memo);
			Servidor::SendToAllButOne(s, mensaje);
		}
	} else if (cmd == "NEWGLINE") {
		if (x.size() != 1) {
			Oper::GlobOPs("GLINE Erroneo.");
			return;
		} else {
			OperServ::ApplyGlines();
		}
	}
	return;
}

void Servidor::SendToAllServers (const string std) {
	for (Servidor *srv = servidores.first(); srv != NULL; srv = servidores.next(srv)) {
		Socket *sock = Servidor::GetSocket(srv->GetNombre());
		if (sock != NULL && srv->GetID() != config->Getvalue("serverID"))
			sock->Write(std + '\n');
	}
}

void Servidor::SendToAllButOne (Socket *s, const string std) {
	for (Servidor *srv = servidores.first(); srv != NULL; srv = servidores.next(srv)) {
		Socket *sock = Servidor::GetSocket(srv->GetNombre());
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
