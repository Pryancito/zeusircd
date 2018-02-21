#include "include.h"
#include "sha256.h"

using namespace std;

bool Oper::Login (User *u, string nickname, string pass) {
	for (unsigned int i = 0; config->Getvalue("oper["+boost::to_string(i)+"]nick").length() > 0; i++)
		if (config->Getvalue("oper["+boost::to_string(i)+"]nick") == nickname)
			if (config->Getvalue("oper["+boost::to_string(i)+"]pass") == sha256(pass)) {
				Oper::SetModeO(u);
				return true;
			}
	Oper::GlobOPs("Intento fallido de autenticacion /oper.");
	return false;
}

void Oper::GlobOPs(string mensaje) {
	for (auto it = users.begin(); it != users.end(); it++)
		if ((*it)->Tiene_Modo('o') == true) {
			Socket *s = User::GetSocketByID((*it)->GetID());
			if (s == NULL)
				Servidor::SendToAllServers(":" + config->Getvalue("serverName") + " NOTICE " + (*it)->GetNick() + " :" + mensaje);
			else
				s->Write(":" + config->Getvalue("serverName") + " NOTICE " + (*it)->GetNick() + " :" + mensaje + "\r\n");	
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
		Socket *s = User::GetSocketByID(u->GetID());
		if (s != NULL) {
			s->Write(":" + config->Getvalue("serverName") + " MODE " + u->GetNick() + " +o\r\n");
			u->Fijar_Modo('o', true);
		}
	}
}

int Oper::Count () {
	int i = 0;
	for (auto it = users.begin(); it != users.end(); it++)
		if ((*it)->Tiene_Modo('o') == true)
			i++;
	return i;
}
