#include "include.h"
#include <algorithm>
#include "../src/lista.cpp"
#include "../src/nodes.cpp"
#include "sha256.h"
#include <deque>

using namespace std;
std::mutex user_mtx;

List <User*> users;
List <Memo*> memos;

User *user = new User();

bool checknick (string nick) {
	if (nick.length() == 0)
		return false;
	for (unsigned int i = 0; i < nick.length(); i++)
		if (!isalnum(nick[i], loc))
			return false;
	return true;
}

bool checkchan (const string chan) {
	if (chan.length() == 0)
		return false;
	for (unsigned int i = 1; i < chan.length(); i++)
		if (!isalnum(chan[i], loc) || chan[0] != '#')
			return false;
	return true;
}

void mayuscula (string &str) {
	for (unsigned int i = 0; i < str.length(); i++)
		str[i] = toupper(str[i]);
}

void User::SetNick(string nick) {
	nickname = nick;
}

string User::GetNick() {
	return nickname;
}

string User::GetID() {
	return id;
}

void User::SetLastPing (time_t tiempo) {
	lastping = tiempo;
}

time_t User::GetLastPing () {
	return lastping;
}

void User::SetIdent(string ident_) {
	ident = ident_;
}

string User::GetIdent() {
	return ident;
}

string User::GetCloakIP() {
	return cloakip;
}

void User::SetCloakIP(string ip) {
	cloakip = ip;
}

string User::GetIP() {
	return ip;
}

void User::SetIP(string ipe) {
	ip = ipe;
}

void User::SetReg(bool reg) {
	registered = reg;
}

bool User::GetReg() {
	return registered;
}

void User::SetNodo(string nodo_) {
	nodo = nodo_;
}

void User::SetLogin (time_t log) {
	login = log;
}

bool User::Tiene_Modo (char modo) {
	switch (modo) {
		case 'o': return tiene_o;
		case 'r': return tiene_r;
		case 'z': return tiene_z;
		case 'w': return tiene_w;
		default: return false;
	}
	return false;
}

void User::Fijar_Modo (char modo, bool tiene) {
	switch (modo) {
		case 'o': tiene_o = tiene; break;
		case 'r': tiene_r = tiene; break;
		case 'z': tiene_z = tiene; break;
		case 'w': tiene_w = tiene; break;
		default: break;
	}
	return;
}

string User::FullNick () {
	string nick;
	nick.append(nickname);
	nick.append("!");
	nick.append(ident);
	nick.append("@");
	nick.append(cloakip);
	return nick;
}

Socket *User::GetSocket(string nickname) {
	for (User *usr = users.first(); usr != NULL; usr = users.next(usr)) {
		if (boost::iequals(usr->GetNick(), nickname, loc)) {
			return usr->socket;
		}
	}
	return NULL;
}

User *User::GetUser(string id) {
	for (User *usr = users.first(); usr != NULL; usr = users.next(usr))
		if (boost::iequals(usr->GetID(), id, loc))
			return usr;
	return NULL;
}

User *User::GetUserByNick(string nickname) {
	for (User *usr = users.first(); usr != NULL; usr = users.next(usr))
		if (boost::iequals(usr->GetNick(), nickname, loc))
			return usr;
	return NULL;
}

Socket *User::GetSocketByID(string id) {
	for (User *usr = users.first(); usr != NULL; usr = users.next(usr)) {
		if (boost::iequals(usr->GetID(), id, loc)) {
			return usr->socket;
		}
	}
	return NULL;
}

string User::GetNickByID(string id) {
	for (User *usr = users.first(); usr != NULL; usr = users.next(usr)) {
		if (boost::iequals(usr->GetID(), id, loc)) {
			return usr->nickname;
		}
	}
	return "";
}

bool User::FindNick(string nickname) {
	for (User *usr = users.first(); usr != NULL; usr = users.next(usr))
		if (boost::iequals(usr->GetNick(), nickname, loc))
			return true;
	return false;
}

time_t User::GetLogin () {
	return login;
}

string User::GetServer () {
	return nodo;
}

bool User::EsMio(string ide) {
	for (User *usr = users.first(); usr != NULL; usr = users.next(usr))
		if (boost::iequals(usr->GetServer(), config->Getvalue("serverName"), loc))
			return true;
	return false;
}

void User::CheckMemos(Socket *s, User *u) {
	deque <Memo *> tmp;
	for (Memo *memo = memos.first(); memo != NULL; memo = memos.next(memo))
		if (boost::iequals(u->GetNick(), memo->receptor, loc)) {
			struct tm *tm = localtime(&memo->time);
			char date[30];
			strftime(date, sizeof(date), "%r %d-%m-%Y", tm);
			string fecha = date;
			s->Write(":MeMo!*@* NOTICE " + u->GetNick() + " :" + date + "\002<" + memo->sender + ">\002 " + memo->mensaje + "\r\n");
			tmp.push_back(memo);
		}
	while (tmp.size() > 0) {
		memos.del(tmp.back());
		tmp.pop_back();
	}
	server->SendToAllServers("MEMODEL " + u->GetNick() + "||");
	return;
}

void User::SendSNICK() {
	string modos = "+";
	if (this->Tiene_Modo('r') == true)
	modos.append("r");
	if (this->Tiene_Modo('z') == true)
	modos.append("z");
	if (this->Tiene_Modo('w') == true)
	modos.append("w");
	if (this->Tiene_Modo('o') == true)
	modos.append("o");
	server->SendToAllServers("SNICK " + this->GetID() + " " + this->GetNick() + " " + this->GetIdent() + " " + this->GetIP() + " " + this->GetCloakIP() + " " + boost::to_string(this->GetLogin()) + " " + this->GetServer() + " " + modos + "||");

}

void User::ProcesaMensaje(Socket *s, string mensaje) {
	if (mensaje.length() == 0 || mensaje == "\r\n" || mensaje == "\r" || mensaje == "\n")
		return;
	vector<string> x;
	boost::split(x,mensaje,boost::is_any_of(" "));
	string cmd = x[0];
	mayuscula(cmd);
	this->SetLastPing(time(0));
	
	if (cmd == "NICK") {
		if (x.size() < 2) {
			s->Write(":" + config->Getvalue("serverName") + " 431 :No has proporcionado un Nick. [ /nick tunick ]" + "\r\n");
			return;
		}
		string nickname = x[1];
		string password = "";
		if (x[1][0] == ':')
			nickname = x[1].substr(1);
		if (nickname.find("!") != std::string::npos || nickname.find(":") != std::string::npos) {
			vector <string> nickpass;
			boost::split(nickpass,nickname,boost::is_any_of(":!"));
			nickname = nickpass[0];
			password = nickpass[1];
		}
		if (checknick(nickname) == false) {
			s->Write(":" + config->Getvalue("serverName") + " 432 :El Nick contiene caracteres no validos." + "\r\n");
			return;
		} else if (nickname == this->GetNick()) {
			return;
		} else if (nickname.length() > (unsigned int )stoi(config->Getvalue("nicklen"))) {
			s->Write(":" + config->Getvalue("serverName") + " 432 :El Nick es demasiado largo." + "\r\n");
			return;
		} else if (boost::iequals(nickname, "ZeusiRCd", loc) || boost::iequals(nickname, "NiCK", loc) || boost::iequals(nickname, "CHaN", loc) || boost::iequals(nickname, "MeMo", loc) || boost::iequals(nickname, "OPeR", loc)) {
			s->Write(":" + config->Getvalue("serverName") + " 432 :El Nick esta reservado." + "\r\n");
			return;
		} else if (boost::iequals(nickname, "NiCKServ", loc) || boost::iequals(nickname, "CHaNServ", loc) || boost::iequals(nickname, "MeMoServ", loc) || boost::iequals(nickname, "OPeRServ", loc)) {
			s->Write(":" + config->Getvalue("serverName") + " 432 :El Nick esta reservado." + "\r\n");
			return;
		} else if (nickserv->IsRegistered(nickname) == true && !boost::iequals(nickname, this->GetNick(), loc)) {
			if (password == "" && !boost::iequals(nickname, this->GetNick(), loc)) {
				s->Write(":NiCK!*@* NOTICE " + nickname + " :No has proporcionado una password. [ /nick tunick:tupass ]" + "\r\n");
				return;
			} else if (nickserv->Login(nickname, password) == 1 && !boost::iequals(nickname, this->GetNick(), loc)) {
				if (this->FindNick(nickname) == true  && !boost::iequals(nickname, this->GetNick(), loc)) {
					User *us = user->GetUserByNick(nickname);
					Socket *sck = user->GetSocket(nickname);
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
							server->SendToAllServers("QUIT " + usr->GetID() + "||");
							break;
						}
					for (Socket *socket = sock.first(); socket != NULL; socket = sock.next(socket))
						if (boost::iequals(socket->GetID(), sck->GetID(), loc)) {
							sck->Close();
							sock.del(socket);
							break;
						}
				}
				if (this->GetNick() == "ZeusiRCd") {
					s->Write(":" + config->Getvalue("serverName") + " MODE " + nickname + " +r\r\n");
					s->Write(":" + nickname + " NICK " + nickname + "\r\n");
					s->Write(":NiCK!*@* NOTICE " + nickname + " :Bienvenido a casa." + "\r\n");
					this->SendSNICK();
					Bienvenida(s, nickname);
					if (s->GetSSL() == 1 && this->Tiene_Modo('z') == false) {
						this->Fijar_Modo('z', true);
						s->Write(":" + config->Getvalue("serverName") + " MODE " + nickname + " +z\r\n");
					}
				} else
					s->Write(":" + this->FullNick() + " NICK " + nickname + "\r\n");
				if (this->Tiene_Modo('r') == false)
					this->Fijar_Modo('r', true);
				s->Write(":" + config->Getvalue("serverName") + " MODE " + nickname + " +r\r\n");

				string modos = "+";
				if (this->Tiene_Modo('r') == true)
					modos.append("r");
				if (this->Tiene_Modo('z') == true)
					modos.append("z");
				if (this->Tiene_Modo('w') == true)
					modos.append("w");
				if (this->Tiene_Modo('o') == true)
					modos.append("o");
				server->SendToAllServers("SVSNICK " + this->GetID() + " " + nickname + "||");

				chan->PropagarNICK(this, nickname);
				this->SetNick(nickname);
				this->CheckMemos(s, this);
				nickserv->UpdateLogin(this);
			} else {
				s->Write(":NiCK!*@* NOTICE " + nickname + " :La password es incorrecta." + "\r\n");
				return;
			}
			return;
		} else {
			if (this->FindNick(nickname) == true && !boost::iequals(nickname, this->GetNick(), loc)) {
				s->Write(":" + config->Getvalue("serverName") + " 433 :El Nick " + nickname + " esta en uso." + "\r\n");
				return;
			} else if (this->GetNick() == "ZeusiRCd") {
				if (s->GetSSL() == 1 && this->Tiene_Modo('z') == false) {
					this->Fijar_Modo('z', true);
					s->Write(":" + config->Getvalue("serverName") + " MODE " + nickname + " +z\r\n");
				}
				s->Write(":" + nickname + " NICK " + nickname + "\r\n");
				this->SendSNICK();
				Bienvenida(s, nickname);
			} else {
				s->Write(":" + this->FullNick() + " NICK " + nickname + "\r\n");
				if (this->Tiene_Modo('r') == true && !boost::iequals(nickname, this->GetNick(), loc)) {
					this->Fijar_Modo('r', false);
					s->Write(":" + config->Getvalue("serverName") + " MODE " + nickname + " -r\r\n");
				}
			}
			server->SendToAllServers("SVSNICK " + this->GetID() + " " + nickname + "||");
			chan->PropagarNICK(this, nickname);
			this->SetNick(nickname);
			return;
		}
	} else if (cmd == "USER") {
		if (x.size() < 2) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :Necesito mas datos. [ USER ident ]" + "\r\n");
			return;
		} else if (checknick(x[1]) == false) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :Tu ident contiene caracteres no validos." + "\r\n");
			return;
		} else if (this->GetReg() == true) {
			s->Write(":" + config->Getvalue("serverName") + " 462 :Ya te has registrado." + "\r\n");
			return;
		} else {
			this->SetIdent(x[1]);
			this->SetReg(true);
			return;
		}
	} else if (cmd == "PING") {
		if (x.size() == 2)
			s->Write(":" + config->Getvalue("serverName") + " PONG :" + x[1] + "\r\n");
		else
			s->Write(":" + config->Getvalue("serverName") + " PONG" + "\r\n");
		this->SetLastPing(time(0));
		return;
	} else if (cmd == "PONG") {
		this->SetLastPing(time(0));
		return;
	} else if (cmd == "QUIT") {
		s->Quit();
		return;
	} else if (cmd == "PRIVMSG" || cmd == "NOTICE") {
		if (x.size() < 2) {
			s->Write(":" + config->Getvalue("serverName") + " 411 :Necesito un destino." + "\r\n");
			return;
		} else if (x.size() < 3) {
			s->Write(":" + config->Getvalue("serverName") + " 412 :Necesito un texto." + "\r\n");
			return;
		} else if (this->GetReg() == false) {
			s->Write(":" + config->Getvalue("serverName") + " 451 :No te has registrado." + "\r\n");
			return;
		} else if (x[1][0] == '#') {
			if (chan->IsInChan(this, x[1]) == 0 || chan->IsBanned(this, x[1]) == 1) {
				s->Write(":" + config->Getvalue("serverName") + " 404 :No puedes enviar texto al canal." + "\r\n");
				return;
			} else {
				chan->PropagarMSG(this, x[1], mensaje + "\r\n");
				server->SendToAllServers(cmd + " " + this->GetID() + mensaje + "||");
				return;
			}
		} else {
			if (nickserv->IsRegistered(this->GetNick()) == 1 && nickserv->GetOption("ONLYREG", x[1]) == 1 && nickserv->IsRegistered(x[1]) == 0) {
				s->Write(":NiCK!*@* NOTICE " + this->GetNick() + " :El nick solo admite privados de usuarios registrados." + "\r\n");
				return;
			}
			for (User *usr = users.first(); usr != NULL; usr = users.next(usr)) {
				if (boost::iequals(usr->GetNick(), x[1], loc)) {
					Socket *sock = user->GetSocketByID(usr->GetID());
					if (sock != NULL) {
						sock->Write(":" + this->FullNick() + " " + mensaje + "\r\n");
						return;
					} else {
						server->SendToAllServers(cmd + " " + this->GetID() + mensaje + "||");
						return;
					}
				}
			} if (nickserv->IsRegistered(x[1]) == 1) {
				int len = 10 + x[1].length();
				string msg = mensaje.substr(len);
				Memo *memo = new Memo();
					memo->sender = this->GetNick();
					memo->receptor = x[1];
					memo->time = time(0);
					memo->mensaje = msg;
				memos.add(memo);
				s->Write(":NiCK!*@* NOTICE " + this->GetNick() + " :El nick no esta conectado, se le ha dejado un MeMo." + "\r\n");
				server->SendToAllServers("MEMO " + memo->sender + " " + memo->receptor + " " + boost::to_string(memo->time) + " " + memo->mensaje);
				return;
			}
			s->Write(":" + config->Getvalue("serverName") + " 401 :El Nick no existe." + "\r\n");
		}
		return;
	} else if (cmd == "JOIN") {
		if (x.size() < 2) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :Necesito mas datos. [ /join #canal ]" + "\r\n");
			return;
		} else if (checkchan(x[1]) == false) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :El Canal contiene caracteres no validos." + "\r\n");
			return;
		} else if (x[1].length() > (unsigned int )stoi(config->Getvalue("chanlen"))) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :El Canal es demasiado largo." + "\r\n");
			return;
		} else if (this->GetReg() == false) {
			s->Write(":" + config->Getvalue("serverName") + " 451 :No te has registrado." + "\r\n");
			return;
		} else if (chan->IsInChan(this, x[1]) == true) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :Ya estas dentro del canal." + "\r\n");
			return;
		} else if (chan->MaxChannels(this) >= stoi(config->Getvalue("maxchannels"))) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :Has entrado en demasiados canales." + "\r\n");
			return;
		} else if (chan->IsBanned(this, x[1]) == 1) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :Estas baneado, no puedes entrar al canal." + "\r\n");
			return;
		} else {
			string mascara = this->GetNick() + "!" + this->GetIdent() + "@" + this->GetCloakIP();
			if (chanserv->IsAKICK(mascara, x[1]) == 1 && oper->IsOper(this) == 0) {
				s->Write(":" + config->Getvalue("serverName") + " 461 :Tienes AKICK en este canal, no puedes entrar." + "\r\n");
				return;
			}
			if (nickserv->IsRegistered(this->GetNick()) == 1) {
				mascara = this->GetNick() + "!" + this->GetIdent() + "@" + nickserv->GetvHost(this->GetNick());
				if (chanserv->IsAKICK(mascara, x[1]) == 1 && oper->IsOper(this) == 0) {
					s->Write(":" + config->Getvalue("serverName") + " 461 :Tienes AKICK en este canal, no puedes entrar." + "\r\n");
					return;
				}
			}
			if (chanserv->IsKEY(x[1]) == true) {
				if (x.size() != 3) {
					s->Write(":" + config->Getvalue("serverName") + " 461 :Necesito mas datos. [ /join #canal password ]" + "\r\n");
					return;
				} else if (chanserv->CheckKEY(x[1], x[2]) == 0) {
					s->Write(":" + config->Getvalue("serverName") + " 461 :Password incorrecto." + "\r\n");
					return;
				}
			}
			chan->Join(this, x[1]);
			chan->PropagarJOIN(this, x[1]);
			chan->SendNAMES(this, x[1]);
			
			if (chanserv->IsRegistered(x[1]) == 1) {
				string sql = "SELECT REGISTERED from CANALES WHERE NOMBRE='" + x[1] + "' COLLATE NOCASE;";
				int registrado = db->SQLiteReturnInt(sql);
				s->Write(":" + config->Getvalue("serverName") + " 329 " + this->GetNick() + " " + x[1] + " " + boost::to_string(registrado) + "\r\n");
				sql = "SELECT TOPIC from CANALES WHERE NOMBRE='" + x[1] + "' COLLATE NOCASE;";
				string topic = db->SQLiteReturnString(sql);
				s->Write(":" + config->Getvalue("serverName") + " 332 " + this->GetNick() + " " + x[1] + " :" + topic + "\r\n");
				if (chan->Tiene_Modo('r') == false) {
					chan->PropagarMODE("CHaN!*@*", "", x[1], 'r', 1);
					chan->Fijar_Modo('r', true);
				}
				chanserv->CheckModes(this, x[1]);
			}
			server->SendToAllServers("SJOIN " + this->GetID() + " " + x[1] + "||");
			return;
		}
	} else if (cmd == "STATS") {
		if (this->GetReg() == false) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return;
		} else {
			s->Write(":" + config->Getvalue("serverName") + " 002 " + this->GetNick() + " :Hay un total de \002" + boost::to_string(sock.count()) + "\002 conexiones y \002" + boost::to_string(usuarios.count()) + "\002 usuarios dentro de canales.\r\n");
			s->Write(":" + config->Getvalue("serverName") + " 002 " + this->GetNick() + " :Hay \002" + boost::to_string(users.count()) + "\002 usuarios y \002" + boost::to_string(canales.count()) + "\002 canales.\r\n");
			s->Write(":" + config->Getvalue("serverName") + " 002 " + this->GetNick() + " :Hay \002" + boost::to_string(nickserv->GetNicks()) + "\002 nicks registrados y \002" + boost::to_string(chanserv->GetChans()) + "\002 canales registrados.\r\n");
			s->Write(":" + config->Getvalue("serverName") + " 002 " + this->GetNick() + " :Hay \002" + boost::to_string(oper->Count()) + "\002 iRCops conectados." + "\r\n");
			s->Write(":" + config->Getvalue("serverName") + " 002 " + this->GetNick() + " :Hay \002" + boost::to_string(servidores.count()) + "\002 servidores conectados." + "\r\n");
			return;
		}
	} else if (cmd == "PART") {
		if (x.size() < 2) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :Necesito mas datos. [ /part #canal ]" + "\r\n");
			return;
		} else if (checkchan(x[1]) == false) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :El Canal contiene caracteres no validos." + "\r\n");
			return;
		} else if (this->GetReg() == false) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return;
		} else if (chan->IsInChan(this, x[1]) == 0) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :No estas dentro del canal." + "\r\n");
			return;
		} else {
			chan->PropagarPART(this, x[1]);
			chan->Part(this, x[1]);
			return;
		}
	} else if (cmd == "OPER") {
		if (x.size() < 3) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :Necesito mas datos." + "\r\n");
			return;
		} else if (this->GetReg() == false) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return;
		} else if (checknick(x[1]) == false) {
			s->Write(":" + config->Getvalue("serverName") + " 432 :El Nick contiene caracteres no validos." + "\r\n");
			return;
		} else if (oper->IsOper(this) == 1) {
			s->Write(":" + config->Getvalue("serverName") + " 381 :Ya eres iRCop." + "\r\n");
			return;
		} else if (oper->Login(this, x[1], x[2]) == 1) {
			server->SendToAllServers("SOPER " + this->GetID());
			s->Write(":" + config->Getvalue("serverName") + " 002 :Has sido identificado como iRCop." + "\r\n");
			return;
		} else {
			s->Write(":" + config->Getvalue("serverName") + " 491 :Identificacion fallida." + "\r\n");
			return;
		}
	} else if (cmd == "REHASH") {
		if (oper->IsOper(this) == 0) {
			s->Write(":" + config->Getvalue("serverName") + " 002 :No tienes privilegios de iRCop." + "\r\n");
			return;
		} else {
			config->Cargar();
			s->Write(":" + config->Getvalue("serverName") + " 002 :La configuracion ha sido recargada." + "\r\n");
			return;
		}
	} else if (cmd == "WEBIRC") {
		if (x.size() < 5) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :Necesito mas datos." + "\r\n");
			return;
		} else if (x[1] == config->Getvalue("cgiirc")) {
			string cloak = sha256(x[4]).substr(0, 16);
			this->SetCloakIP(cloak);
			this->SetIP(x[4]);
			this->Fijar_Modo('w', true);
			return;
		}
	} else if (cmd == "LIST") {
		if (this->GetReg() == false) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return;
		} else if (x.size() == 1) {
			chan->Lista("*", this);
			return;
		} else if (x.size() == 2) {
			chan->Lista(x[1], this);
			return;
		} else
			return;
	} else if (cmd == "KICK") {
		if (x.size() < 3) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :Necesito mas datos. [ /kick #canal nick (motivo) ]" + "\r\n");
			return;
		} else if (this->GetReg() == false) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return;
		} else if (chan->IsInChan(this, x[1]) == 0) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :No estas dentro del canal." + "\r\n");
			return;
		} else if (user->FindNick(x[2]) == 0) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :El nick no existe." + "\r\n");
			return;
		} else if (chan->IsInChan(user->GetUserByNick(x[2]), x[1]) == 0) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :El nick no esta dentro del canal." + "\r\n");
			return;
		} else {
			bool can = false;
			string motivo = ":Has sido expulsado del canal.";
			for (UserChan *uc = usuarios.first(); uc != NULL; uc = usuarios.next(uc))
				if (boost::iequals(uc->GetNombre(), x[1], loc))
					if (boost::iequals(uc->GetID(), this->GetID(), loc))
						if (uc->GetModo() == 'o' || uc->GetModo() == 'h')
							can = true;
			if (can == false) {
				s->Write(":" + config->Getvalue("serverName") + " 461 :No tienes status de operador (+o) ni de halfop (+h)." + "\r\n");
				return;
			}
			if (x.size() > 3) {
				int posicion = 4 + cmd.length() + x[1].length() + x[2].length();
				motivo = ":" + mensaje.substr(posicion);
			}
			User *usr = user->GetUserByNick(x[2]);
			chan->PropagarKICK(this, x[1], usr, motivo);
			chan->Part(usr, x[1]);
			server->SendToAllServers("SKICK " + this->GetID() + " " + x[1] + " " + usr->GetID() + " " + motivo + "||");
			return;
		}
	} else if (cmd == "SERVERS") {
		if (this->GetReg() == false) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return;
		} else if (oper->IsOper(this) == 0) {
			s->Write(":" + config->Getvalue("serverName") + " 002 :No tienes privilegios de iRCop." + "\r\n");
			return;
		} else if (oper->IsOper(this) == 1) {
			server->ListServers(s);
			return;
		} else return;
	} else if (cmd == "CONNECT") {
		if (x.size() < 3) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :Necesito mas datos." + "\r\n");
			return;
		} else if (oper->IsOper(this) == 0) {
			s->Write(":" + config->Getvalue("serverName") + " 002 :No tienes privilegios de iRCop." + "\r\n");
			return;
		} else if (server->IsAServer(x[1]) == 0) {
			s->Write(":" + config->Getvalue("serverName") + " 002 :El servidor no esta en mi lista." + "\r\n");
			return;
		} else if (server->IsConected(x[1]) == 1) {
			s->Write(":" + config->Getvalue("serverName") + " 002 :El servidor ya esta conectado." + "\r\n");
			return;
		} else {
			s->Write(":" + config->Getvalue("serverName") + " 002 :Conectando..." + "\r\n");
			socket->Conectar(x[1], x[2]);
			return;
		}
	} else if (cmd == "SQUIT") {
		if (x.size() < 2) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :Necesito mas datos." + "\r\n");
			return;
		} else if (oper->IsOper(this) == 0) {
			s->Write(":" + config->Getvalue("serverName") + " 002 :No tienes privilegios de iRCop." + "\r\n");
			return;
		} else if (server->Existe(x[1]) == 0) {
			s->Write(":" + config->Getvalue("serverName") + " 002 :El servidor no esta conectado." + "\r\n");
			return;
		} else if (server->Existe(x[1]) == 1) {
			server->SQUIT(x[1]);
			return;
		}
	} else if (cmd == "WHOIS") {
		if (x.size() < 2) {
			s->Write(":" + config->Getvalue("serverName") + " 401 " + this->GetNick() + " " + x[1] + " :Necesito mas datos. [ /whois nick ]" + "\r\n");
			s->Write(":" + config->Getvalue("serverName") + " 318 " + this->GetNick() + " " + x[1] + " :Fin de /WHOIS." + "\r\n");
			return;
		} else if (this->GetReg() == false) {
			s->Write(":" + config->Getvalue("serverName") + " 401 " + this->GetNick() + " " + x[1] + " :No te has registrado." + "\r\n");
			s->Write(":" + config->Getvalue("serverName") + " 318 " + this->GetNick() + " " + x[1] + " :Fin de /WHOIS." + "\r\n");
			return;
		} else {
			User *usr = user->GetUserByNick(x[1]);
			string sql;
			if (usr == NULL && nickserv->IsRegistered(x[1]) == 1) {
				s->Write(":" + config->Getvalue("serverName") + " 320 " + this->GetNick() + " " + x[1] + " :STATUS: \0034DESCONECTADO\003.\r\n");
				s->Write(":" + config->Getvalue("serverName") + " 320 " + this->GetNick() + " " + x[1] + " :Tiene el nick registrado.\r\n");
				sql = "SELECT SHOWMAIL FROM OPTIONS WHERE NICKNAME='" + x[1] + "' COLLATE NOCASE;";
				if (db->SQLiteReturnInt(sql) == 1) {
					sql = "SELECT EMAIL FROM NICKS WHERE NICKNAME='" + x[1] + "' COLLATE NOCASE;";
					string email = db->SQLiteReturnString(sql);
					if (email.length() > 0)
					s->Write(":" + config->Getvalue("serverName") + " 320 " + this->GetNick() + " " + x[1] + " :Su correo electronico es: " + email + "\r\n");
				}
				sql = "SELECT URL FROM NICKS WHERE NICKNAME='" + x[1] + "' COLLATE NOCASE;";
				string url = db->SQLiteReturnString(sql);
				if (url.length() > 0)
					s->Write(":" + config->Getvalue("serverName") + " 320 " + this->GetNick() + " " + x[1] + " :Su Web es: " + url + "\r\n");
				sql = "SELECT VHOST FROM NICKS WHERE NICKNAME='" + x[1] + "' COLLATE NOCASE;";
				string vHost = db->SQLiteReturnString(sql);
				if (vHost.length() > 0)
					s->Write(":" + config->Getvalue("serverName") + " 320 " + this->GetNick() + " " + x[1] + " :Su vHost es: " + vHost + "\r\n");
				sql = "SELECT REGISTERED FROM NICKS WHERE NICKNAME='" + x[1] + "' COLLATE NOCASE;";
				int registro = db->SQLiteReturnInt(sql);
				if (registro > 0) {
					string tiempo = user->Time(registro);
					s->Write(":" + config->Getvalue("serverName") + " 320 " + this->GetNick() + " " + x[1] + " :Registrado desde: " + tiempo + "\r\n");
				}
				sql = "SELECT LASTUSED FROM NICKS WHERE NICKNAME='" + x[1] + "' COLLATE NOCASE;";
				int last = db->SQLiteReturnInt(sql);
				string tiempo = user->Time(last);
				if (tiempo.length() > 0 && last > 0)
					s->Write(":" + config->Getvalue("serverName") + " 320 " + this->GetNick() + " " + x[1] + " :Visto por ultima vez: " + tiempo + "\r\n");
				s->Write(":" + config->Getvalue("serverName") + " 318 " + this->GetNick() + " " + x[1] + " :Fin de /WHOIS." + "\r\n");
				return;
			} else if (usr != NULL && nickserv->IsRegistered(x[1]) == 1) {
				s->Write(":" + config->Getvalue("serverName") + " 320 " + this->GetNick() + " " + usr->GetNick() + " :" + usr->GetNick() + " es: " + usr->GetNick() + "!" + usr->GetIdent() + "@" + usr->GetCloakIP() + "\r\n");
				s->Write(":" + config->Getvalue("serverName") + " 320 " + this->GetNick() + " " + usr->GetNick() + " :STATUS: \0033CONECTADO\003.\r\n");
				s->Write(":" + config->Getvalue("serverName") + " 320 " + this->GetNick() + " " + usr->GetNick() + " :Tiene el nick registrado.\r\n");
				if (oper->IsOper(this) == 1)
					s->Write(":" + config->Getvalue("serverName") + " 320 " + this->GetNick() + " " + usr->GetNick() + " :Su IP es: " + usr->GetIP() + "\r\n");
				if (oper->IsOper(usr) == 1)
					s->Write(":" + config->Getvalue("serverName") + " 320 " + this->GetNick() + " " + usr->GetNick() + " :Es un iRCop.\r\n");
				if (usr->Tiene_Modo('z') == 1)
					s->Write(":" + config->Getvalue("serverName") + " 320 " + this->GetNick() + " " + usr->GetNick() + " :Conecta mediante un canal seguro SSL.\r\n");
				if (usr->Tiene_Modo('w') == 1)
					s->Write(":" + config->Getvalue("serverName") + " 320 " + this->GetNick() + " " + usr->GetNick() + " :Conecta mediante WebChat.\r\n");
				if (nickserv->GetOption("SHOWMAIL", usr->GetNick()) == 1) {
					sql = "SELECT EMAIL FROM NICKS WHERE NICKNAME='" + usr->GetNick() + "' COLLATE NOCASE;";
					string email = db->SQLiteReturnString(sql);
					if (email.length() > 0)
					s->Write(":" + config->Getvalue("serverName") + " 320 " + this->GetNick() + " " + usr->GetNick() + " :Su correo electronico es: " + email + "\r\n");
				}
				sql = "SELECT URL FROM NICKS WHERE NICKNAME='" + usr->GetNick() + "' COLLATE NOCASE;";
				string url = db->SQLiteReturnString(sql);
				if (url.length() > 0)
					s->Write(":" + config->Getvalue("serverName") + " 320 " + this->GetNick() + " " + usr->GetNick() + " :Su Web es: " + url + "\r\n");
				sql = "SELECT VHOST FROM NICKS WHERE NICKNAME='" + usr->GetNick() + "' COLLATE NOCASE;";
				string vHost = db->SQLiteReturnString(sql);
				if (vHost.length() > 0)
					s->Write(":" + config->Getvalue("serverName") + " 320 " + this->GetNick() + " " + usr->GetNick() + " :Su vHost es: " + vHost + "\r\n");
				if (this == usr && nickserv->IsRegistered(this->GetNick()) == 1) {
					string opciones;
					if (nickserv->GetOption("NOACCESS", this->GetNick()) == 1) {
						if (!opciones.empty())
							opciones.append(", ");
						opciones.append("NOACCESS");
					}
					if (nickserv->GetOption("SHOWMAIL", this->GetNick()) == 1) {
						if (!opciones.empty())
							opciones.append(", ");
						opciones.append("SHOWMAIL");
					}
					if (nickserv->GetOption("NOMEMO", this->GetNick()) == 1) {
						if (!opciones.empty())
							opciones.append(", ");
						opciones.append("NOMEMO");
					}
					if (nickserv->GetOption("NOOP", this->GetNick()) == 1) {
						if (!opciones.empty())
							opciones.append(", ");
						opciones.append("NOOP");
					}
					if (nickserv->GetOption("ONLYREG", this->GetNick()) == 1) {
						if (!opciones.empty())
							opciones.append(", ");
						opciones.append("ONLYREG");
					}
					if (opciones.length() == 0)
						opciones = "Ninguna";
					s->Write(":" + config->Getvalue("serverName") + " 320 " + this->GetNick() + " " + usr->GetNick() + " :Tus opciones son: " + opciones + "\r\n");
				}
				sql = "SELECT REGISTERED FROM NICKS WHERE NICKNAME='" + usr->GetNick() + "' COLLATE NOCASE;";
				int registro = db->SQLiteReturnInt(sql);
				if (registro > 0) {
					string tiempo = user->Time(registro);
					s->Write(":" + config->Getvalue("serverName") + " 320 " + this->GetNick() + " " + usr->GetNick() + " :Registrado desde: " + tiempo + "\r\n");
				}
				string tiempo = user->Time(usr->GetLogin());
				if (tiempo.length() > 0)
					s->Write(":" + config->Getvalue("serverName") + " 320 " + this->GetNick() + " " + usr->GetNick() + " :Entro hace: " + tiempo + "\r\n");
				s->Write(":" + config->Getvalue("serverName") + " 318 " + this->GetNick() + " " + usr->GetNick() + " :Fin de /WHOIS." + "\r\n");
				return;
			} else if (usr != NULL && nickserv->IsRegistered(x[1]) == 0) {
				s->Write(":" + config->Getvalue("serverName") + " 320 " + this->GetNick() + " " + usr->GetNick() + " :" + usr->GetNick() + " es: " + usr->GetNick() + "!" + usr->GetIdent() + "@" + usr->GetCloakIP() + "\r\n");
				s->Write(":" + config->Getvalue("serverName") + " 320 " + this->GetNick() + " " + usr->GetNick() + " :STATUS: \0033CONECTADO\003.\r\n");
				if (oper->IsOper(this) == 1)
					s->Write(":" + config->Getvalue("serverName") + " 320 " + this->GetNick() + " " + usr->GetNick() + " :Su IP es: " + usr->GetIP() + "\r\n");
				if (oper->IsOper(usr) == 1)
					s->Write(":" + config->Getvalue("serverName") + " 320 " + this->GetNick() + " " + usr->GetNick() + " :Es un iRCop.\r\n");
				if (usr->Tiene_Modo('z') == 1)
					s->Write(":" + config->Getvalue("serverName") + " 320 " + this->GetNick() + " " + usr->GetNick() + " :Conecta mediante un canal seguro SSL.\r\n");
				if (usr->Tiene_Modo('w') == 1)
					s->Write(":" + config->Getvalue("serverName") + " 320 " + this->GetNick() + " " + usr->GetNick() + " :Conecta mediante WebChat.\r\n");
				string tiempo = user->Time(usr->GetLogin());
				if (tiempo.length() > 0)
					s->Write(":" + config->Getvalue("serverName") + " 320 " + this->GetNick() + " " + usr->GetNick() + " :Entro hace: " + tiempo + "\r\n");
				s->Write(":" + config->Getvalue("serverName") + " 318 " + this->GetNick() + " " + usr->GetNick() + " :Fin de /WHOIS." + "\r\n");
				return;
			} else {
				s->Write(":" + config->Getvalue("serverName") + " 401 " + this->GetNick() + " " + x[1] + " :El nick no existe." + "\r\n");
				s->Write(":" + config->Getvalue("serverName") + " 318 " + this->GetNick() + " " + x[1] + " :Fin de /WHOIS." + "\r\n");
				return;
			}
		}
	} else if (cmd == "MODE") {
		if (x.size() < 2) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :Necesito mas datos. [ /mode #canal (+modo) (nick) ]" + "\r\n");
			return;
		} else if (this->GetReg() == false) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return;
		} else if (x[1][0] == '#') {
			if (chanserv->IsRegistered(x[1]) == 0) {
				s->Write(":" + config->Getvalue("serverName") + " 461 :El canal no esta registrado." + "\r\n");
				return;
			} else if (chan->IsInChan(this, x[1]) == 0) {
				s->Write(":" + config->Getvalue("serverName") + " 461 :No estas dentro del canal." + "\r\n");
				return;
			} else if (x.size() == 2) {
				string sql = "SELECT MODOS from CANALES WHERE NOMBRE='" + x[1] + "' COLLATE NOCASE;";
				string modos = db->SQLiteReturnString(sql);
				s->Write(":" + config->Getvalue("serverName") + " 324 " + this->GetNick() + " " + x[1] + " " + modos + "\r\n");
				return;
			} else if (x.size() == 3) {
				if (x[2] == "+b" || x[2] == "b") {
					for (BanChan *bc = bans.first(); bc != NULL; bc = bans.next(bc))
						if (boost::iequals(bc->GetNombre(), x[1], loc))
							s->Write(":" + config->Getvalue("serverName") + " 367 " + this->GetNick() + " " + x[1] + " " + bc->GetMask() + " " + bc->GetWho() + " " + boost::to_string(bc->GetTime()) + "\r\n");
					s->Write(":" + config->Getvalue("serverName") + " 368 " + this->GetNick() + " " + x[1] + " :Fin de la lista de baneados." + "\r\n");
				}
			} else if (x.size() > 3) {
				if (chanserv->Access(this->GetNick(), x[1]) < 2) {
					s->Write(":" + config->Getvalue("serverName") + " 461 :No tienes acceso para banear." + "\r\n");
					return;
				}
				bool action = 0, match = 0;
				unsigned int j = 0;
				string ban;
				string msg = mensaje.substr(5);
				char mode;
				for (unsigned int i = 0; i < x[2].length(); i++) {
					if (x[2][i] == '+') {
						action = 1;
						mode = '+';
					} else if (x[2][i] == '-') {
						action = 0;
						mode = '-';
					} else if (x[2][i] == 'b') {
						vector <string> baneos;
						string maskara = x[3+j];
						boost::algorithm::to_lower(maskara);
						for (BanChan *bc = bans.first(); bc != NULL; bc = bans.next(bc))
							if (boost::iequals(bc->GetNombre(), x[1], loc))
								baneos.push_back(bc->GetMask());
								
						if (baneos.size() == 0) {
							if (action == 1) {
								chan->ChannelBan(this->GetNick(), maskara, x[1]);
								chan->PropagarMODE(this->FullNick(), maskara, x[1], 'b', action);
								s->Write(":CHaN!*@* NOTICE " + this->GetNick() + " :El BAN ha sido fijado." + "\r\n");
								server->SendToAllServers("SMODE " + this->GetNick() + " " + x[1] + " " + mode + "b " + x[3+j]);
							} else {
								s->Write(":CHaN!*@* NOTICE " + this->GetNick() + " :El canal no tiene BANs." + "\r\n");
							}
						} else {
							for (unsigned int i = 0; i < baneos.size(); i++) {
								boost::algorithm::to_lower(baneos[i]);
								if (user->Match(baneos[i].c_str(), maskara.c_str()) == 1) {
									match = 1;
									ban = baneos[i];						
								}
							}
							if (match == 0) {
								if (action == 0) {
									s->Write(":CHaN!*@* NOTICE " + this->GetNick() + " :El BAN no existe." + "\r\n");
								} else {
									chan->ChannelBan(this->GetNick(), maskara, x[1]);
									chan->PropagarMODE(this->FullNick(), maskara, x[1], 'b', action);
									server->SendToAllServers("SMODE " + this->GetNick() + " " + x[1] + " " + mode + "b " + maskara);
									s->Write(":CHaN!*@* NOTICE " + this->GetNick() + " :El BAN ha sido fijado." + "\r\n");
								}
							} else {
								if (action == 1) {
									s->Write(":CHaN!*@* NOTICE " + this->GetNick() + " :El BAN ya existe como " + ban + "\r\n");
								} else {
									chan->UnBan(ban, x[1]);
									chan->PropagarMODE(this->FullNick(), x[3+j], x[1], 'b', action);
									server->SendToAllServers("SMODE " + this->GetNick() + " " + x[1] + " " + mode + "b " + maskara);
									s->Write(":CHaN!*@* NOTICE " + this->GetNick() + " :El BAN ha sido eliminado." + "\r\n");
								}
							}
						}
						j++;
					}
				}
			}
		}
	} else if (cmd == "VERSION") {
		s->Write(":" + config->Getvalue("serverName") + " 002 " + this->GetNick() + " :Tu Nodo es: " + config->Getvalue("serverName") + " funcionando con: " + config->version + "\r\n");
		return;
	} else if (cmd == "UPTIME") {
		struct tm *tm = localtime(&encendido);
		char date[30];
		strftime(date, sizeof(date), "%r %d-%m-%Y", tm);
		string fecha = date;	
		s->Write(":" + config->Getvalue("serverName") + " 003 " + this->GetNick() + " :Este servidor fue creado: " + fecha + "\r\n");
		return;
	} else if (cmd == "NICKSERV" || cmd == "NS") {
		if (this->GetReg() == false) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return;
		} else if (x.size() < 2) {
			s->Write(":NiCK!*@* NOTICE " + this->GetNick() + " :Necesito mas datos. [ /nickserv help ]" + "\r\n");
			return;
		} else {
			nickserv->ProcesaMensaje(s, this, mensaje.substr(9));
			return;
		}
	} else if (cmd == "CHANSERV" || cmd == "CS") {
		if (this->GetReg() == false) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return;
		} else if (x.size() < 2) {
			s->Write(":CHaN!*@* NOTICE " + this->GetNick() + " :Necesito mas datos. [ /chanserv help ]" + "\r\n");
			return;
		} else {
			chanserv->ProcesaMensaje(s, this, mensaje.substr(9));
			return;
		}
	} else if (cmd == "OPERSERV" || cmd == "OS") {
		if (this->GetReg() == false) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return;
		} else if (oper->IsOper(this) == 0) {
			s->Write(":" + config->Getvalue("serverName") + " 461 :No eres un operador." + "\r\n");
			return;
		} else if (x.size() < 2) {
			s->Write(":OPeR!*@* NOTICE " + this->GetNick() + " :Necesito mas datos. [ /operserv help ]" + "\r\n");
			return;
		} else {
			operserv->ProcesaMensaje(s, this, mensaje.substr(9));
			return;
		}
	}
}

void User::Quit(User *u, Socket *s) {
	vector <UserChan*> temp;
	boost::thread *trd;
	for (UserChan *uc = usuarios.first(); uc != NULL; uc = usuarios.next(uc)) {
		if (boost::iequals(uc->GetID(), u->GetID(), loc)) {
			std::lock_guard<std::mutex> lock(user_mtx);
			chan->PropagarQUIT(u, uc->GetNombre());
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
		if (boost::iequals(usr->GetID(), u->GetID(), loc)) {
			users.del(usr);
			server->SendToAllServers("QUIT " + usr->GetID() + "||");
			break;
		}
	for (Socket *socket = sock.first(); socket != NULL; socket = sock.next(socket))
		if (boost::iequals(socket->GetID(), s->GetID(), loc)) {
			socket->Close();
			sock.del(socket);
			break;
		}
	delete u;
	trd = std::move(s->tw);
	delete s;
	delete trd;
	return;
}

void User::Bienvenida(Socket *s, string nickname) {
	struct tm *tm = localtime(&encendido);
	char date[30];
	strftime(date, sizeof(date), "%r %d-%m-%Y", tm);
	string fecha = date;
	s->Write(":" + config->Getvalue("serverName") + " 001 " + nickname + " :Bienvenido a " + config->Getvalue("network") + "\r\n");
	s->Write(":" + config->Getvalue("serverName") + " 002 " + nickname + " :Tu Nodo es: " + config->Getvalue("serverName") + " funcionando con: " + config->version + "\r\n");
	s->Write(":" + config->Getvalue("serverName") + " 003 " + nickname + " :Este servidor fue creado: " + fecha + "\r\n");
	s->Write(":" + config->Getvalue("serverName") + " 004 " + nickname + " " + config->Getvalue("serverName") + " " + config->version + " rzoiws robtkmlvshn r\r\n");
	s->Write(":" + config->Getvalue("serverName") + " 005 " + nickname + " NETWORK=" + config->Getvalue("network") + " are supported by this server\r\n");
	s->Write(":" + config->Getvalue("serverName") + " 005 " + nickname + " NICKLEN=" + config->Getvalue("nicklen") + " MAXCHANNELS=" + config->Getvalue("maxchannels") + " CHANNELLEN=" + config->Getvalue("chanlen") + " are supported by this server\r\n");
	s->Write(":" + config->Getvalue("serverName") + " 005 " + nickname + " PREFIX=(ohv)@%+ STATUSMSG=@%+ are supported by this server\r\n");
	s->Write(":" + config->Getvalue("serverName") + " 002 " + nickname + " :Hay un total de \002" + boost::to_string(sock.count()) + "\002 conexiones y \002" + boost::to_string(usuarios.count()) + "\002 usuarios dentro de canales.\r\n");
	s->Write(":" + config->Getvalue("serverName") + " 002 " + nickname + " :Hay \002" + boost::to_string(users.count()) + "\002 usuarios y \002" + boost::to_string(canales.count()) + "\002 canales.\r\n");
	s->Write(":" + config->Getvalue("serverName") + " 002 " + nickname + " :Hay \002" + boost::to_string(nickserv->GetNicks()) + "\002 nicks registrados y \002" + boost::to_string(chanserv->GetChans()) + "\002 canales registrados.\r\n");
	s->Write(":" + config->Getvalue("serverName") + " 002 " + nickname + " :Hay \002" + boost::to_string(oper->Count()) + "\002 iRCops conectados." + "\r\n");
	s->Write(":" + config->Getvalue("serverName") + " 002 " + nickname + " :Hay \002" + boost::to_string(servidores.count()) + "\002 servidores conectados." + "\r\n");
}

bool User::Match(const char *first, const char *second)
{
    // If we reach at the end of both strings, we are done
    if (*first == '\0' && *second == '\0')
        return true;
 
    // Make sure that the characters after '*' are present
    // in second string. This function assumes that the first
    // string will not contain two consecutive '*'
    if (*first == '*' && *(first+1) != '\0' && *second == '\0')
        return false;
 
    // If the first string contains '?', or current characters
    // of both strings match
    if (*first == '?' || *first == *second)
        return Match(first+1, second+1);
 
    // If there is *, then there are two possibilities
    // a) We consider current character of second string
    // b) We ignore current character of second string.
    if (*first == '*')
        return Match(first+1, second) || Match(first, second+1);
    return false;
}

string User::Time(time_t tiempo) {
	int dias, horas, minutos, segundos = 0;
	string total;
	time_t now = time(0);
	tiempo = now - tiempo;
	if (tiempo <= 0)
		return "0s";
	dias = tiempo / 86400;
	tiempo = tiempo - ( dias * 86400 );
	horas = tiempo / 3600;
	tiempo = tiempo - ( horas * 3600 );
	minutos = tiempo / 60;
	tiempo = tiempo - ( minutos * 60 );
	segundos = tiempo;
	
	if (dias) {
		total.append(boost::to_string(dias));
		total.append("d ");
	} if (horas) {
		total.append(boost::to_string(horas));
		total.append("h ");
	} if (minutos) {
		total.append(boost::to_string(minutos));
		total.append("m ");
	} if (segundos) {
		total.append(boost::to_string(segundos));
		total.append("s");
	}
	return total;
}
