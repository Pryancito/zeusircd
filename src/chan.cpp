#include "include.h"
#include <algorithm>
#include <deque>

using namespace std;

list <Chan*> canales;


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
	for (auto it = canales.begin(); it != canales.end(); it++)
        if (boost::iequals((*it)->GetNombre(), kanal))
			return true;
	return false;
}

Chan *Chan::GetChan(string kanal) {
	for (auto it = canales.begin(); it != canales.end(); it++)
        if (boost::iequals((*it)->GetNombre(), kanal))
			return (*it);
	return nullptr;
}

bool Chan::IsInChan (User *u, string canal) {
	for (auto it = u->channels.begin(); it != u->channels.end(); it++)
		if (boost::iequals((*it)->GetNombre(), canal))
			return true;
	return false;
}

int Chan::MaxChannels(User *u) {
	return u->CountChannels();
}

bool Chan::IsBanned(User *u, string canal) {
	string fullnick = "";
	Chan *channel = Chan::GetChan(canal);
	if (channel == NULL)
		return false;
	for (auto it = channel->chanbans.begin(); it != channel->chanbans.end(); it++) {
		string mask = (*it)->GetMask();
		if (NickServ::IsRegistered(u->GetNick()) == 1) {
			fullnick = u->GetNick() + "!" + u->GetIdent() + "@" + NickServ::GetvHost(u->GetNick());
			boost::algorithm::to_lower(fullnick);
			boost::algorithm::to_lower(mask);
			if (User::Match(mask.c_str(), fullnick.c_str()) == 1)
				return true;
		}
		fullnick = u->GetNick() + "!" + u->GetIdent() + "@" + u->GetCloakIP();
		boost::algorithm::to_lower(fullnick);
		boost::algorithm::to_lower(mask);
		if (User::Match(mask.c_str(), fullnick.c_str()) == 1)
			return true;
	}
	return false;
}

int Chan::GetUsers(string canal) {
	for (auto it = canales.begin(); it != canales.end(); it++)
        if (boost::iequals((*it)->GetNombre(), canal))
        	return (*it)->chanusers.size();
    return 0;
}

void Chan::DelChan(string canal) {
	for (auto it = canales.begin(); it != canales.end(); it++)
        if (boost::iequals((*it)->GetNombre(), canal)) {
            canales.erase(it);
            return;
    	}
}

void Chan::Join (User *u, string canal) {
	Chan *c;
	if (Chan::FindChan(canal) == false) {
		c = new Chan(canal);
		canales.push_back(c);
	} else
		c = Chan::GetChan(canal);
	UserChan *uc = new UserChan (u->GetID(), canal);
	c->chanusers.push_back(uc);
	u->channels.push_back(c);
	return;
}

void Chan::Part (User *u, string canal) {
	Chan *c = Chan::GetChan(canal);
	for (auto it = c->chanusers.begin(); it != c->chanusers.end(); it++)
	    if ((*it)->GetID() == u->GetID()) {
	    	c->chanusers.erase(it);
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
	Chan *channel = Chan::GetChan(canal);
	//boost::lock_guard<boost::mutex> lock(channel->mtx);
	for (auto it = channel->chanusers.begin(); it != channel->chanusers.end(); it++) {
		Socket *s = User::GetSocketByID((*it)->id);
		if (s != NULL)
			s->Write(":" + u->FullNick() + " JOIN :" + channel->GetNombre() + "\r\n");
	}
}

void Chan::PropagarPART (User *u, string canal) {
	Chan *channel = Chan::GetChan(canal);
	//boost::lock_guard<boost::mutex> lock(channel->mtx);
	for (auto it = channel->chanusers.begin(); it != channel->chanusers.end(); it++) {
		Socket *s = User::GetSocketByID((*it)->id);
		if (s != NULL)
			s->Write(":" + u->FullNick() + " PART " + channel->GetNombre() + "\r\n");
	}
}

void Chan::PropagarQUIT (User *u, string canal) {
	Chan *channel = Chan::GetChan(canal);
	//boost::lock_guard<boost::mutex> lock(channel->mtx);
	for (auto it = channel->chanusers.begin(); it != channel->chanusers.end(); it++) {
		Socket *s = User::GetSocketByID((*it)->id);
		if (s != NULL)
			s->Write(":" + u->FullNick() + " QUIT :QUIT" + "\r\n");
	}
}

void Chan::SendNAMES (User *u, string canal) {
	string names;
	Socket *sock = u->GetSocket();
	Chan *channel = Chan::GetChan(canal);
	//boost::lock_guard<boost::mutex> lock(channel->mtx);
	for (auto it = channel->chanusers.begin(); it != channel->chanusers.end(); it++) {
			if (!names.empty())
				names.append(" ");
			if ((*it)->GetModo() == 'v')
				names.append("+");
			else if ((*it)->GetModo() == 'h')
				names.append("%");
			else if ((*it)->GetModo() == 'o')
				names.append("@");
			names.append(User::GetNickByID((*it)->id));
			if (names.length() > 450) {
				if (sock != NULL)
					sock->Write(":" + config->Getvalue("serverName") + " 353 " + u->GetNick() + " = " + channel->GetNombre() + " :" + names + "\r\n");
				names.clear();
			}
	}
	if (sock != NULL) {
		if (names.length() > 0)
			sock->Write(":" + config->Getvalue("serverName") + " 353 " + u->GetNick() + " = " + channel->GetNombre() + " :" + names + "\r\n");
		sock->Write(":" + config->Getvalue("serverName") + " 366 " + u->GetNick() + " " + channel->GetNombre() + " :End of /NAMES list\r\n");
	}
}

void Chan::SendWHO (User *u, string canal) {
	Socket *s = u->GetSocket();
	Chan *channel = Chan::GetChan(canal);
	//boost::lock_guard<boost::mutex> lock(channel->mtx);
	for (auto it = channel->chanusers.begin(); it != channel->chanusers.end(); it++) {
			User *user = User::GetUser((*it)->id);
			string modo = "H";
			if (Oper::IsOper(user))
				modo.append("*");
			if ((*it)->GetModo() == 'v')
				modo.append("+");
			else if ((*it)->GetModo() == 'h')
				modo.append("%");
			else if ((*it)->GetModo() == 'o')
				modo.append("@");
			if (s != NULL) {
				if (!NickServ::GetvHost(user->GetNick()).empty())
					s->Write(":" + config->Getvalue("serverName") + " 352 " + u->GetNick() + " " + channel->GetNombre() + " " + user->GetIdent() + " " + NickServ::GetvHost(user->GetNick()) + " *.* " + user->GetNick() + " " + modo + " :0 ZeusiRCd" + "\r\n");
				else
					s->Write(":" + config->Getvalue("serverName") + " 352 " + u->GetNick() + " " + channel->GetNombre() + " " + user->GetIdent() + " " + user->GetCloakIP() + " *.* " + user->GetNick() + " " + modo + " :0 ZeusiRCd" + "\r\n");
			}
	}
	s->Write(":" + config->Getvalue("serverName") + " 315 " + u->GetNick() + " " + channel->GetNombre() + " :End of /WHO list\r\n");
}

void Chan::PropagarMSG(User *u, string canal, string mensaje) {
	Chan *channel = Chan::GetChan(canal);
	//boost::lock_guard<boost::mutex> lock(channel->mtx);
	for (auto it = channel->chanusers.begin(); it != channel->chanusers.end(); it++) {
		Socket *s = User::GetSocketByID((*it)->id);
		if (s != NULL && (*it)->id != u->GetID())
			s->Write(":" + u->FullNick() + " " + mensaje);
	}
}

void Chan::Lista (std::string canal, User *u) {
	if (canal.length() == 0)
		canal = "*";
	string nickname = u->GetNick();
	Socket *s = u->GetSocket();
	s->Write(":" + config->Getvalue("serverName") + " 321 " + nickname + " Channel :Lista de canales." + "\r\n");

	for (auto it = canales.begin(); it != canales.end(); it++) {
		string topic = "";
		string mtch = (*it)->GetNombre();
		boost::algorithm::to_lower(canal);
		boost::algorithm::to_lower(mtch);
		if (User::Match(canal.c_str(), mtch.c_str()) == 1) {
			if (ChanServ::IsRegistered(mtch) == 1) {
				string sql = "SELECT TOPIC from CANALES WHERE NOMBRE='" + mtch + "' COLLATE NOCASE;";
				topic = DB::SQLiteReturnString(sql);
			}
			s->Write(":" + config->Getvalue("serverName") + " 322 " + nickname + " " + (*it)->GetNombre() + " " + boost::to_string(Chan::GetUsers(mtch)) + " :" + topic + "\r\n");
		}
	}
	
	s->Write(":" + config->Getvalue("serverName") + " 323 " + nickname + " :Fin de /LIST." + "\r\n");
	return;
}

void Chan::PropagarMODE(string who, string nickname, string canal, char modo, bool add, bool propagate) {
	char simbol;
	string id;
	if (add == 1)
		simbol = '+';
	else
		simbol = '-';
	Chan *channel = Chan::GetChan(canal);
	//boost::lock_guard<boost::mutex> lock(channel->mtx);
	for (auto it = channel->chanusers.begin(); it != channel->chanusers.end(); it++) {
		Socket *s = User::GetSocketByID((*it)->id);
		if (s != NULL) {
			if (modo == 'b') {
					s->Write(":" + who + " MODE " + (*it)->GetNombre() + " " + simbol + modo + " " + nickname + "\r\n");
			} else {
				if (nickname.length() == 0) {
					s->Write(":" + who + " MODE " + (*it)->GetNombre() + " " + simbol + modo + "\r\n");
					continue;
				}
				string id = User::GetUserByNick(nickname)->GetID();
				if ((*it)->GetID() == id) {
					if (add == 0)
						(*it)->SetModo('x');
					else
						(*it)->SetModo(modo);
				}
				if (s != NULL)
					s->Write(":" + who + " MODE " + channel->GetNombre() + " " + simbol + modo + " " + nickname + "\r\n");
			}
		}
	}
	if (propagate)
		Servidor::SendToAllServers("SMODE " + who + " " + channel->GetNombre() + " " + simbol + modo + " " + nickname);
	return;
}

void Chan::PropagarNICK(User *u, string nuevo) {
	for (auto it = u->channels.begin(); it != u->channels.end(); it++) {
		Chan *channel = Chan::GetChan((*it)->GetNombre());
		//boost::lock_guard<boost::mutex> lock(channel->mtx);
		for (auto it2 = channel->chanusers.begin(); it2 != channel->chanusers.end(); it2++)
			if ((*it2)->id != u->GetID()) {
				Socket *s = User::GetSocketByID((*it2)->GetID());
				if (s != NULL)
					s->Write(":" + u->FullNick() + " NICK :" + nuevo + "\r\n");
			}
				
	}
}

void Chan::PropagarKICK (User *u, string canal, User *user, string motivo) {
	Chan *channel = Chan::GetChan(canal);
	//boost::lock_guard<boost::mutex> lock(channel->mtx);
	for (auto it = channel->chanusers.begin(); it != channel->chanusers.end(); it++) {
		Socket *s = User::GetSocketByID((*it)->id);
		if (s != NULL)
			s->Write(":" + u->FullNick() + " KICK " + channel->GetNombre() + " " + user->GetNick() + " " + motivo + "\r\n");
	}
}

void Chan::ChannelBan(string who, string mascara, string channel) {
	BanChan *b = new BanChan(channel, mascara, who, time(0));
	Chan *chan = Chan::GetChan(channel);
	chan->chanbans.push_back(b);
}

void Chan::UnBan(string mascara, string canal) {
	Chan *channel = Chan::GetChan(canal);
	for (auto it = channel->chanbans.begin(); it != channel->chanbans.end(); it++)
		if (boost::iequals((*it)->GetMask(), mascara)) {
			channel->chanbans.erase(it);
        	return;
		}
}

