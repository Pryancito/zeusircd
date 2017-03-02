#include "include.h"

using namespace std;

Nick *nick = new Nick();

bool Nick::Existe(string _nick) {
	int id = datos->BuscarIDNick(_nick);
	if (id < 0)
		return false;
	return true;
}

long int Nick::Creado(string _nick) {
	int id = datos->BuscarIDNick(_nick);
	if (id < 0)
		return false;
	return datos->nicks[id]->login;
}

bool Nick::Conectado(int ID) {
	if (ID < 0)
		return false;
	return datos->nicks[ID]->conectado;
}

bool Nick::Registrado(int ID) {
	if (ID < 0)
		return false;
	return datos->nicks[ID]->registrado;
}

void Nick::CambioDeNick(int ID, string newnick) {
	if (ID < 0)
		return;
	datos->nicks[ID]->nickname = newnick;
	return;
}

void Nick::Conectar (int ID) {
	if (ID < 0)
		return;
	datos->nicks[ID]->conectado = true;
	return;
}

string Nick::GetNick(int ID) {
	if (ID < 0)
		return "";
	return datos->nicks[ID]->nickname;
}

string Nick::GetIdent(int ID) {
	if (ID < 0)
		return "";
	return datos->nicks[ID]->ident;
}

string Nick::GetCloakIP(int ID) {
	if (ID < 0)
		return "";
	return datos->nicks[ID]->cloakip;
}

string Nick::GetIP(int ID) {
	if (ID < 0)
		return "";
	return datos->nicks[ID]->ip;
}

void Nick::SetIdent(int ID, string _ident) {
	if (ID < 0)
		return;
	datos->nicks[ID]->ident = _ident;
	datos->nicks[ID]->registrado = true;
	return;
}

string Nick::FullNick(int ID) {
	if (ID < 0)
		return "";

	string nickname = GetNick(ID);
	nickname.append("!");
	nickname.append(GetIdent(ID));
	nickname.append("@");
	nickname.append(GetCloakIP(ID));
	return nickname;
}

