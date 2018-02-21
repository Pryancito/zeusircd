#include "include.h"
#include "sha256.h"
#include <vector>

using namespace std;

list <Servidor*> servidores;
std::mutex servidores_mtx;

string Servidor::GetIP () {
	return ip;
}

void Servidor::SQUIT(Servidor *s) {
	vector <string> servers;
	for (auto it = servidores.begin(); it != servidores.end(); it++) {
		if (Servidor::GetSocket((*it)->GetNombre())->GetID() == Servidor::GetSocket(s->GetNombre())->GetID()) {
			servers.push_back((*it)->GetNombre());
			for (unsigned int i = 0; i < (*it)->connected.size(); i++) {
				servers.push_back((*it)->connected[i]);
			}
		}
	}
	for (unsigned int i = 0; i < servers.size(); i++)
		Servidor::SQUIT(servers[i]);
}

void Servidor::SQUIT(Socket *s) {
	vector <string> servers;
	for (auto it = servidores.begin(); it != servidores.end(); it++) {
		if (Servidor::GetSocket((*it)->GetNombre())->GetID() == s->GetID()) {
			servers.push_back((*it)->GetNombre());
			for (unsigned int i = 0; i < (*it)->connected.size(); i++) {
				servers.push_back((*it)->connected[i]);
			}
		}
	}
	for (unsigned int i = 0; i < servers.size(); i++)
		Servidor::SQUIT(servers[i]);
}

void Servidor::SQUIT(string nombre) {
	vector <UserChan*> temp;
	vector <User *> usuar;
	for (auto it = servidores.begin(); it != servidores.end(); it++)
		if (boost::iequals((*it)->GetNombre(), nombre)) {
			for (auto it2 = users.begin(); it2 != users.end(); it2++) {
				if (boost::iequals((*it2)->GetServer(), (*it)->GetNombre())) {
					Socket *s = (*it2)->GetSocket();
					for (auto it3 = (*it2)->channels.begin(); it3 != (*it2)->channels.end(); it3++) {
						Chan::PropagarQUIT(*it2, (*it3)->GetNombre());
						for (auto it4 = (*it2)->channels.begin(); it4 != (*it2)->channels.end(); it4++)
			        		if (boost::iequals((*it3)->GetNombre(), (*it4)->GetNombre())) {
			        			(*it2)->channels.erase(it4);
			        			break;
							}
						Chan::Part(*it2, (*it3)->GetNombre());
					}
					if (User::EsMio((*it2)->GetID()) == 1) {
						Servidor::SendToAllServers("QUIT " + (*it2)->GetID());
						s->Close();
						users.erase(it2);
					}
				}
			}
			Servidor::SendToAllServers("SQUIT " + (*it)->GetID() + " " + config->Getvalue("serverID"));
			Servidor::GetSocket((*it)->GetNombre())->Close();
			servidores.erase(it);
		}
}

Socket *Servidor::GetSocket(string nombre) {
	for (auto it = servidores.begin(); it != servidores.end(); it++)
		if (boost::iequals((*it)->GetNombre(), nombre))
			return (*it)->socket;
	return NULL;
}

Servidor *Servidor::GetServer(string id) {
	for (auto it = servidores.begin(); it != servidores.end(); it++)
		if ((*it)->GetID() == id)
			return *it;
	return NULL;
}

bool Servidor::Existe(string id) {
	for (auto it = servidores.begin(); it != servidores.end(); it++)
		if ((*it)->GetID() == id)
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
	for (auto it = servidores.begin(); it != servidores.end(); it++)
		if (boost::iequals((*it)->GetIP(), ip))
			return true;
	return false;
}

bool Servidor::HUBExiste() {
	for (auto it = servidores.begin(); it != servidores.end(); it++)
		if (boost::iequals((*it)->GetNombre(), config->Getvalue("hub")))
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
	for (auto it = servidores.begin(); it != servidores.end(); it++) {
		string servidor = "SERVER " + (*it)->GetID() + " " + (*it)->GetNombre() + " " + (*it)->GetIP() + " 0";
		for (unsigned int i = 0; i < (*it)->connected.size(); i++) {
			servidor.append(" ");
			servidor.append((*it)->connected[i]);
		}
		servidor.append("\n");
		s->Write(servidor);
	}
	for (auto it = users.begin(); it != users.end(); it++) {
		string modos = "+";
		if ((*it)->Tiene_Modo('r') == true)
			modos.append("r");
		if ((*it)->Tiene_Modo('z') == true)
			modos.append("z");
		if ((*it)->Tiene_Modo('w') == true)
			modos.append("w");
		if ((*it)->Tiene_Modo('o') == true)
			modos.append("o");
		s->Write("SNICK " + (*it)->GetID() + " " + (*it)->GetNick() + " " + (*it)->GetIdent() + " " + (*it)->GetIP() + " " + (*it)->GetCloakIP() + " " + boost::to_string((*it)->GetLogin()) + " " + (*it)->GetServer() + " " + modos + "\n");
	}
	for (auto it = canales.begin(); it != canales.end(); it++) {
		for (auto it2 = (*it)->chanbans.begin(); it2 != (*it)->chanbans.end(); it2++)
			s->Write("SBAN " + (*it2)->GetNombre() + " " + (*it2)->GetMask() + " " + (*it2)->GetWho() + " " + boost::to_string((*it2)->GetTime()) + "\n");
		for (auto it2 = (*it)->chanusers.begin(); it2 != (*it)->chanusers.end(); it2++)
			s->Write("SJOIN " + (*it2)->GetID() + " " + (*it2)->GetNombre() + " " + (*it2)->GetModo() +  "\n");
	}
	for (auto it = memos.begin(); it != memos.end(); it++)
		s->Write("MEMO " + (*it)->sender + " " + (*it)->receptor + " " + boost::to_string((*it)->time) + " " + (*it)->mensaje + "\n");

	OperServ::ApplyGlines();
	return;
}

void Servidor::ListServers (Socket *s) {
	for (auto it = servidores.begin(); it != servidores.end(); it++)
		s->Write(":" + config->Getvalue("serverName") + " 002 :ID: " + (*it)->GetID() + " NOMBRE: " + (*it)->GetNombre() + " ( " + (*it)->GetIP() + " )" + "\r\n");
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
		} else if (!boost::iequals(x[1], config->Getvalue("hub"))) {
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
			servidores.push_back(xs);
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
			users.push_back(u);
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
			users.push_back(u);
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
			Chan *channel = Chan::GetChan(x[1]);
			for (auto it = channel->chanbans.begin(); it != channel->chanbans.end(); it++) {
				std::string baneo = (*it)->GetMask();
				std::string mascara = x[2];
				boost::algorithm::to_lower(baneo);
				boost::algorithm::to_lower(mascara);
				if (User::Match(baneo.c_str(), mascara.c_str()) == 1) {
					return;
				}
			}
			std::time_t time = (long ) atoi(x[4].c_str());
			BanChan *b = new BanChan(x[1], x[2], x[3], time);
			channel->chanbans.push_back(b);
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
				for (auto it = dest->channels.begin(); it != dest->channels.end(); it++)
	        		if (boost::iequals((*it)->GetNombre(), x[2])) {
	        			dest->channels.erase(it);
	        			break;
					}
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
			User *u = User::GetUser(x[1]);
			for (auto it = u->channels.begin(); it != u->channels.end();) {
				Chan::PropagarQUIT(u, (*it)->GetNombre());
				Chan::Part(u, (*it)->GetNombre());
				it = u->channels.erase(it);
			}
			for (auto it = users.begin(); it != users.end(); it++)
				if ((*it)->GetID() == u->GetID()) {
					users.erase(it);
					break;
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
			User *ust = User::GetUserByNick(x[2]);
			Socket *sock = User::GetSocketByID(ust->GetID());
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
				Servidor::SendToAllServers("SKICK " + usr->GetID() + " " + x[2] + " " + usr->GetID() + " Estas baneado, no puedes entrar al canal.");
				return;
			}
			else if (ChanServ::IsAKICK(mascara, x[2]) == 1 && Oper::IsOper(usr) == 0) {
				Servidor::SendToAllServers("SKICK " + usr->GetID() + " " + x[2] + " " + usr->GetID() + " Tienes AKICK en este canal, no puedes entrar.");
				return;
			}
			else if (NickServ::IsRegistered(usr->GetNick()) == 1 && !NickServ::GetvHost(usr->GetNick()).empty()) {
				mascara = usr->GetNick() + "!" + usr->GetIdent() + "@" + NickServ::GetvHost(usr->GetNick());
				if (ChanServ::IsAKICK(mascara, x[2]) == 1 && Oper::IsOper(usr) == 0) {
					Servidor::SendToAllServers("SKICK " + usr->GetID() + " " + x[2] + " " + usr->GetID() + " Tienes AKICK en este canal, no puedes entrar.");
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
			for (auto it = usr->channels.begin(); it != usr->channels.end(); it++)
        		if (boost::iequals((*it)->GetNombre(), x[2])) {
        			usr->channels.erase(it);
        			break;
				}
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
			for (auto it = memos.begin(); it != memos.end(); it++)
				if (boost::iequals(x[1], (*it)->sender) && boost::iequals(x[2], (*it)->receptor) && boost::iequals(x[3], boost::to_string((*it)->time)) && boost::iequals(msg, (*it)->mensaje))
					return;
			Memo *memo = new Memo();
				memo->sender = x[1];
				memo->receptor = x[2];
				memo->time = stoi(x[3]);
				memo->mensaje = msg;
			memos.push_back(memo);
			Servidor::SendToAllButOne(s, mensaje);
		}
	} else if (cmd == "MEMODEL") {
		if (x.size() != 2) {
			Oper::GlobOPs("MEMODEL Erroneo.");
			return;
		} else {
			for (auto it = memos.begin(); it != memos.end(); it++)
				if (boost::iequals(x[1], (*it)->receptor))
					memos.erase(it);
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
	for (auto it = servidores.begin(); it != servidores.end(); it++) {
		Socket *s = Servidor::GetSocket((*it)->GetNombre());
		if (s != NULL && (*it)->GetID() != config->Getvalue("serverID"))
			s->Write(std + '\n');
	}
}

void Servidor::SendToAllButOne (Socket *s, const string std) {
	for (auto it = servidores.begin(); it != servidores.end(); it++) {
		Socket *sock = Servidor::GetSocket((*it)->GetNombre());
		if (sock != NULL && sock->GetID() != s->GetID() && (*it)->GetID() != config->Getvalue("serverID"))
			sock->Write(std + '\n');
	}
}

bool Servidor::CheckClone(string ip) {
	int count = 1;
	for (auto it = users.begin(); it != users.end(); it++)
		if (boost::iequals((*it)->GetIP(), ip))
			count++;
	if (count > stoi(config->Getvalue("clones")))
		return true;
	else
		return false;
}
