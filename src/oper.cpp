#include "include.h"
#include "sha256.h"

using namespace std;

Oper *oper = new Oper();

bool Oper::Login (User *u, string nickname, string pass) {
	for (unsigned int i = 0; config->Getvalue("oper["+boost::to_string(i)+"]nick").length() > 0; i++)
		if (config->Getvalue("oper["+boost::to_string(i)+"]nick") == nickname)
			if (config->Getvalue("oper["+boost::to_string(i)+"]pass") == sha256(pass)) {
				oper->SetModeO(u);
				return true;
			}
	return false;
}

void Oper::GlobOPs(string mensaje) {
	for (User *usr = users.first(); usr != NULL; usr = users.next(usr)) {
		if (usr->Tiene_Modo('o') == true) {
			Socket *sock = user->GetSocket(usr->GetNick());
			if (sock == NULL)
				server->SendToAllServers(":" + config->Getvalue("serverName") + " NOTICE " + usr->GetNick() + " :" + mensaje + "\r\n");
			else
				sock->Write(":" + config->Getvalue("serverName") + " NOTICE " + usr->GetNick() + " :" + mensaje + "\r\n");	
		}
	}
}

string Oper::MkPassWD (string pass) {
	return sha256(pass);
}

bool Oper::IsOper(User *u) {
	return u->Tiene_Modo('o');
}

void Oper::SetModeO (User *u) {
	if (u->Tiene_Modo('o') == false) {
		Socket *s = u->GetSocket(u->GetNick());
		if (s != NULL) {
			s->Write(":" + config->Getvalue("serverName") + " MODE " + u->GetNick() + " +o\r\n");
			u->Fijar_Modo('o', true);
		}
	}
}

int Oper::Count () {
	int i = 0;
	for (User *usr = users.first(); usr != NULL; usr = users.next(usr))
		if (usr->Tiene_Modo('o') == true)
			i++;
	return i;
}
