#include "include.h"
#include "sha256.h"

using namespace std;

Oper *oper = new Oper();

bool Oper::Login (string source, string nickname, string pass) {
	for (unsigned int i = 0; config->Getvalue("oper["+to_string(i)+"]nick").length() > 0; i++)
		if (config->Getvalue("oper["+to_string(i)+"]nick") == nickname)
			if (config->Getvalue("oper["+to_string(i)+"]pass") == sha256(pass)) {
				datos->SetOper(source);
				return true;
			}
	return false;
}

void Oper::GlobOPs(string mensaje) {
	for (unsigned int i = 0; i < datos->operadores.size(); i++) {
		TCPStream *stream = datos->BuscarStream(datos->operadores[i]->nickoper);
		if (stream == NULL)
			server->SendToAllServers(":" + config->Getvalue("serverName") + " NOTICE " + datos->operadores[i]->nickoper + " :" + mensaje + "\r\n");
		else
			sock->Write(stream, ":" + config->Getvalue("serverName") + " NOTICE " + datos->operadores[i]->nickoper + " :" + mensaje + "\r\n");
	}
}

string Oper::MkPassWD (string pass) {
	return sha256(pass);
}

bool Oper::IsOper(string nick) {
	for (unsigned int i = 0; i < datos->operadores.size(); i++)
		if (mayus(datos->operadores[i]->nickoper) == mayus(nick))
			return true;
	return false;
}
