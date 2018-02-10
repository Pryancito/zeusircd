#include "include.h"
#include <algorithm>
#include "../src/lista.cpp"
#include "../src/nodes.cpp"

using namespace std;

List<Chan*> canales;
List<UserChan*> usuarios;
List<BanChan*> bans;

std::mutex chan_mtx;

string Chan::GetNombre() {
	return nombre;
}

string BanChan::GetNombre () {
	return canal;
}

string BanChan::GetMask () {
	return mascara;
}

string BanChan::GetWho () {
	return who;
}

time_t BanChan::GetTime () {
	return fecha;
}

bool Chan::FindChan(string kanal) {
	for (Chan *canal = canales.first(); canal != NULL; canal = canales.next(canal))
		if (boost::iequals(canal->GetNombre(), kanal, loc))
			return true;
	return false;
}

Chan *Chan::GetChan(string kanal) {
	for (Chan *canal = canales.first(); canal != NULL; canal = canales.next(canal))
		if (boost::iequals(canal->GetNombre(), kanal, loc))
			return canal;
	return NULL;
}

bool Chan::IsInChan (User *u, string canal) {
	for (UserChan *uc = usuarios.first(); uc != NULL; uc = usuarios.next(uc))
		if (boost::iequals(uc->GetNombre(), canal, loc) && boost::iequals(uc->GetID(), u->GetID(), loc))
			return true;
	return false;
}

int Chan::MaxChannels(User *u) {
	int chan = 0;
	for (UserChan *uc = usuarios.first(); uc != NULL; uc = usuarios.next(uc))
		if (boost::iequals(uc->GetID(), u->GetID(), loc))
			chan++;
	return chan;
}

bool Chan::IsBanned(User *u, string canal) {
	string fullnick = "";
	for (BanChan *bc = bans.first(); bc != NULL; bc = bans.next(bc)) {
		if (NickServ::IsRegistered(u->GetNick()) == 1) {
			fullnick = u->GetNick() + "!" + u->GetIdent() + "@" + NickServ::GetvHost(u->GetNick());
			boost::algorithm::to_lower(fullnick);
			if (User::Match(bc->GetMask().c_str(), fullnick.c_str()) == 1)
				return true;
		}
		fullnick = u->GetNick() + "!" + u->GetIdent() + "@" + u->GetCloakIP();
		boost::algorithm::to_lower(fullnick);
		if (User::Match(bc->GetMask().c_str(), fullnick.c_str()) == 1)
			return true;
	}
	return false;
}

int Chan::GetUsers(string canal) {
	int i = 0;
	for (UserChan *uc = usuarios.first(); uc != NULL; uc = usuarios.next(uc))
		if (boost::iequals(uc->GetNombre(), canal, loc))
			i++;
	return i;
}

void Chan::DelChan(string canal) {
	for (Chan *c = canales.first(); c != NULL; c = canales.next(c))
		if (boost::iequals(c->GetNombre(), canal, loc))
			canales.del(c);
}

void Chan::Join (User *u, string canal) {
	if (Chan::FindChan(canal) == false) {
		Chan *c = new Chan(canal);
		canales.add(c);
	}
	UserChan *uc = new UserChan (u->GetID(), canal);
	usuarios.add(uc);
	return;
}

void Chan::Part (User *u, string canal) {
	for (UserChan *uc = usuarios.first(); uc != NULL; uc = usuarios.next(uc))
		if (boost::iequals(uc->GetNombre(), canal, loc) && boost::iequals(uc->GetID(), u->GetID(), loc)) {
			usuarios.del(uc);
			break;
		}
			
	if (Chan::GetUsers(canal) == 0) {
		Chan::DelChan(canal);
	}
} 

string UserChan::GetID() {
	return id;
}

char UserChan::GetModo() {
	return modo;
}

void UserChan::SetModo(char mode) {
	modo = mode;
}

string UserChan::GetNombre() {
	return canal;
}

boost::mutex &UserChan::GetMTX() {
	return mtx;
}

bool Chan::Tiene_Modo(char modo) {
	switch (modo) {
		case 'r': return tiene_r;
		default: return false;
	}
	return false;
}

void Chan::Fijar_Modo(char modo, bool tiene) {
	switch (modo) {
		case 'r': tiene_r = tiene; break;
		default: break;
	}
	return;
}

void Chan::PropagarJOIN (User *u, string canal) {
	for (UserChan *uc = usuarios.first(); uc != NULL; uc = usuarios.next(uc)) {
		boost::mutex::scoped_lock lock(uc->GetMTX());
		if (boost::iequals(uc->GetNombre(), canal, loc)) {
			Socket *sock = User::GetSocketByID(uc->GetID());
			if (sock != NULL)
				sock->Write(":" + u->FullNick() + " JOIN :" + uc->GetNombre() + "\r\n");
		}
	}
}

void Chan::PropagarPART (User *u, string canal) {
	for (UserChan *uc = usuarios.first(); uc != NULL; uc = usuarios.next(uc)) {
		boost::mutex::scoped_lock lock(uc->GetMTX());
		if (boost::iequals(uc->GetNombre(), canal, loc)) {
			Socket *sock = User::GetSocketByID(uc->GetID());
			if (sock != NULL)
				sock->Write(":" + u->FullNick() + " PART " + uc->GetNombre() + "\r\n");
		}
	}
}

void Chan::PropagarQUIT (User *u, string canal) {
	for (UserChan *uc = usuarios.first(); uc != NULL; uc = usuarios.next(uc)) {
		boost::mutex::scoped_lock lock(uc->GetMTX());
		if (boost::iequals(uc->GetNombre(), canal, loc)) {
			Socket *sock = User::GetSocketByID(uc->GetID());
			if (sock != NULL)
				sock->Write(":" + u->FullNick() + " QUIT :QUIT" + "\r\n");
		}
	}
}

void Chan::SendNAMES (User *u, string canal) {
	string names;
	Socket *sock = User::GetSocketByID(u->GetID());
	for (UserChan *uc = usuarios.first(); uc != NULL; uc = usuarios.next(uc)) {
		boost::mutex::scoped_lock lock(uc->GetMTX());
		if (boost::iequals(uc->GetNombre(), canal, loc) && User::GetNickByID(uc->GetID()) != "") {
			if (!names.empty())
				names.append(" ");
			if (uc->GetModo() == 'v')
				names.append("+");
			else if (uc->GetModo() == 'h')
				names.append("%");
			else if (uc->GetModo() == 'o')
				names.append("@");
			names.append(User::GetNickByID(uc->GetID()));
			if (names.length() > 450) {
				if (sock != NULL)
					sock->Write(":" + config->Getvalue("serverName") + " 353 " + u->GetNick() + " = " + canal + " :" + names + "\r\n");
				names.clear();
			}
				
		}
	}
	if (sock != NULL) {
		if (names.length() > 0)
			sock->Write(":" + config->Getvalue("serverName") + " 353 " + u->GetNick() + " = " + canal + " :" + names + "\r\n");
		sock->Write(":" + config->Getvalue("serverName") + " 366 " + u->GetNick() + " " + canal + " :End of /NAMES list\r\n");
	}
}

void Chan::SendWHO (User *u, string canal) {
	Socket *sock = User::GetSocketByID(u->GetID());
	for (UserChan *uc = usuarios.first(); uc != NULL; uc = usuarios.next(uc)) {
		boost::mutex::scoped_lock lock(uc->GetMTX());
		if (boost::iequals(uc->GetNombre(), canal, loc) && User::GetNickByID(uc->GetID()) != "") {
			User *user = User::GetUser(uc->GetID());
			string modo = "H";
			if (Oper::IsOper(user))
				modo.append("*");
			if (uc->GetModo() == 'v')
				modo.append("+");
			else if (uc->GetModo() == 'h')
				modo.append("%");
			else if (uc->GetModo() == 'o')
				modo.append("@");
			if (sock != NULL) {
				if (!NickServ::GetvHost(user->GetNick()).empty())
					sock->Write(":" + config->Getvalue("serverName") + " 352 " + u->GetNick() + " " + canal + " " + user->GetIdent() + " " + NickServ::GetvHost(user->GetNick()) + " *.* " + user->GetNick() + " " + modo + " :0 ZeusiRCd" + "\r\n");
				else
					sock->Write(":" + config->Getvalue("serverName") + " 352 " + u->GetNick() + " " + canal + " " + user->GetIdent() + " " + user->GetCloakIP() + " *.* " + user->GetNick() + " " + modo + " :0 ZeusiRCd" + "\r\n");
			}
		}
	}
	sock->Write(":" + config->Getvalue("serverName") + " 315 " + u->GetNick() + " " + canal + " :End of /WHO list\r\n");
}

void Chan::PropagarMSG(User *u, string canal, string mensaje) {
	for (UserChan *uc = usuarios.first(); uc != NULL; uc = usuarios.next(uc)) {
		boost::mutex::scoped_lock lock(uc->GetMTX());
		if (boost::iequals(uc->GetNombre(), canal, loc) && uc->GetID() != u->GetID()) {
			Socket *sock = User::GetSocketByID(uc->GetID());
			if (sock != NULL)
				sock->Write(":" + u->FullNick() + " " + mensaje);
		}
	}
}

void Chan::Lista (std::string canal, User *u) {
	if (canal.length() == 0)
		canal = "*";
	string nickname = u->GetNick();
	Socket *sock = User::GetSocket(nickname);
	sock->Write(":" + config->Getvalue("serverName") + " 321 " + nickname + " Channel :Lista de canales." + "\r\n");

	for (Chan *c = canales.first(); c != NULL; c = canales.next(c)) {
		string topic = "";
		string mtch = c->GetNombre();
		boost::algorithm::to_lower(canal);
		boost::algorithm::to_lower(mtch);
		if (User::Match(canal.c_str(), mtch.c_str()) == 1) {
			if (ChanServ::IsRegistered(mtch) == 1) {
				string sql = "SELECT TOPIC from CANALES WHERE NOMBRE='" + mtch + "' COLLATE NOCASE;";
				topic = DB::SQLiteReturnString(sql);
			}
			sock->Write(":" + config->Getvalue("serverName") + " 322 " + nickname + " " + c->GetNombre() + " " + boost::to_string(Chan::GetUsers(mtch)) + " :" + topic + "\r\n");
		}
	}
	
	sock->Write(":" + config->Getvalue("serverName") + " 323 " + nickname + " :Fin de /LIST." + "\r\n");
	return;
}

void Chan::PropagarMODE(string who, string nickname, string canal, char modo, bool add, bool propagate) {
	char simbol;
	string id;
	if (add == 1)
		simbol = '+';
	else
		simbol = '-';
	for (UserChan *uc = usuarios.first(); uc != NULL; uc = usuarios.next(uc)) {
		boost::mutex::scoped_lock lock(uc->GetMTX());
		if (boost::iequals(uc->GetNombre(), canal, loc)) {
			if (modo == 'b') {
				Socket *sock = User::GetSocketByID(uc->GetID());
				if (sock != NULL)
					sock->Write(":" + who + " MODE " + uc->GetNombre() + " " + simbol + modo + " " + nickname + "\r\n");
			} else {
				if (nickname.length() == 0) {
					Socket *sock = User::GetSocketByID(uc->GetID());
					if (sock != NULL)
						sock->Write(":" + who + " MODE " + uc->GetNombre() + " " + simbol + modo + "\r\n");
					continue;
				}
				string id = User::GetUserByNick(nickname)->GetID();
				if (boost::iequals(uc->GetID(), id, loc)) {
					if (add == 0)
						uc->SetModo('x');
					else
						uc->SetModo(modo);
				}
				Socket *sock = User::GetSocketByID(uc->GetID());
				if (sock != NULL)
					sock->Write(":" + who + " MODE " + uc->GetNombre() + " " + simbol + modo + " " + nickname + "\r\n");
			}
		}
	}
	if (propagate)
		Servidor::SendToAllServers("SMODE " + who + " " + canal + " " + simbol + modo + " " + nickname);
	return;
}

void Chan::PropagarNICK(User *u, string nuevo) {
	for (UserChan *uc = usuarios.first(); uc != NULL; uc = usuarios.next(uc)) {
		boost::mutex::scoped_lock lock(uc->GetMTX());
		if (boost::iequals(uc->GetID(), u->GetID(), loc)) {
			for (UserChan *uc2 = usuarios.first(); uc2 != NULL; uc2 = usuarios.next(uc2)) {
				if (boost::iequals(uc->GetNombre(), uc2->GetNombre(), loc)) {
					Socket *sock = User::GetSocketByID(uc2->GetID());
					if (sock != NULL && !boost::iequals(uc2->GetID(), u->GetID(), loc))
						sock->Write(":" + u->FullNick() + " NICK :" + nuevo + "\r\n");
				}
			}
		}
	}
}

void Chan::PropagarKICK (User *u, string canal, User *user, string motivo) {
	for (UserChan *uc = usuarios.first(); uc != NULL; uc = usuarios.next(uc)) {
		boost::mutex::scoped_lock lock(uc->GetMTX());
		if (boost::iequals(uc->GetNombre(), canal, loc)) {
			Socket *sock = User::GetSocketByID(uc->GetID());
			if (sock != NULL)
				sock->Write(":" + u->FullNick() + " KICK " + uc->GetNombre() + " " + User::GetNickByID(uc->GetID()) + " " + motivo + "\r\n");
		}
	}
}

void Chan::ChannelBan(string who, string mascara, string channel) {
	BanChan *b = new BanChan(channel, mascara, who, time(0));
	bans.add(b);
}

void Chan::UnBan(string mascara, string channel) {
	for (BanChan *bc = bans.first(); bc != NULL; bc = bans.next(bc))
		if (boost::iequals(bc->GetNombre(), channel, loc) && boost::iequals(bc->GetMask(), mascara, loc)) {
			bans.del(bc);
			return;
		}
}

