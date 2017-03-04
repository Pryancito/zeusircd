#include "include.h"
#include "sha256.h"

using namespace std;

Data *datos = new Data();

mutex sock_mute, nick_mute, chan_mute, server_mute, oper_mute;

void Data::CrearNick(TCPStream *stream, string _nick) {
	Nick *nickinfo = new Nick();
		nickinfo->stream = stream;
		nickinfo->nickname = _nick;
		nickinfo->ident = "ZeusiRCd";
		nickinfo->login = static_cast<long int> (time(NULL));
		nickinfo->nodo = config->Getvalue("serverName");
		nickinfo->conectado = true;
		nickinfo->cloakip = sha256(stream->getPeerIP()).substr(0, 16);
		nickinfo->ip = stream->getPeerIP();
	nick_mute.lock();
	datos->nicks.push_back(nickinfo);
	nick_mute.unlock();
	return;
}

int Data::BuscarIDNick(string nick) {
	for (unsigned int i = 0; i < datos->nicks.size(); i++)
		if (mayus(datos->nicks[i]->nickname) == mayus(nick))
			return i;
	return -1;
}

int Data::BuscarIDStream(TCPStream *stream) {
	for (unsigned int i = 0; i < datos->nicks.size(); i++)
		if (datos->nicks[i]->stream == stream)
			return i;
	return -1;
}

void Data::DelOper(string nick) {
	for (unsigned int i = 0; i < datos->operadores.size(); i++)
		if (mayus(datos->operadores[i]->nickoper) == mayus(nick)) {
			oper_mute.lock();
			datos->operadores.erase(datos->operadores.begin() + i);
		}
}

void Data::BorrarNick(TCPStream *stream) {
	int id = datos->BuscarIDStream(stream);
	if (id < 0)
		return;
	if (oper->IsOper(datos->nicks[id]->nickname) == 1)
		datos->DelOper(datos->nicks[id]->nickname);
	nick_mute.lock();
	datos->nicks.erase(datos->nicks.begin() + id);
	nick_mute.unlock();
	return;
}

void Data::BorrarNickByNick(string nickname) {
	int id = datos->BuscarIDNick(nickname);
	if (id < 0)
		return;
	if (oper->IsOper(nickname) == 1)
		datos->DelOper(nickname);
	nick_mute.lock();
	datos->nicks.erase(datos->nicks.begin() + id);
	nick_mute.unlock();
	return;
}

TCPStream *Data::BuscarStream(string nick) {
	int id = datos->BuscarIDNick(nick);
	if (id < 0)
		return NULL;
	return datos->nicks[id]->stream;
}

void Data::AddServer (TCPStream* stream, string nombre, string ip, int saltos) {
	Server *servidor = new Server();
		servidor->stream = stream;
		servidor->nombre = nombre;
		servidor->ip = ip;
		servidor->saltos = saltos;
		servidor->hub = config->Getvalue("hub");
	server_mute.lock();
	datos->servers.push_back(servidor);
	server_mute.unlock();
}

void Data::DeleteServer (string name) {
	int id = 0;
	for (unsigned int i = 0; i < datos->servers.size(); i++) {
		if (datos->servers[i]->nombre == name) {
			id = i;
			break;
		}
	}
	server_mute.lock();
	datos->servers.erase(datos->servers.begin() + id);
	server_mute.unlock();
}

void Data::Conexiones(string principal, string linkado) {
	for (unsigned int i = 0; i < datos->servers.size(); i++)
		if (principal == datos->servers[i]->nombre) {
			server_mute.lock();
			datos->servers[i]->connected.push_back(linkado);
			server_mute.unlock();	
		}
}

void Data::CrearCanal(string nombre) {
	Chan *canal = new Chan();
		canal->nombre = nombre;
		canal->modos = "+nt";
		canal->creado = static_cast<long int> (time(NULL));
	chan_mute.lock();
	datos->canales.push_back(canal);
	chan_mute.unlock();
}

void Data::AddUsersToChan(int id, string nickname) {
	chan_mute.lock();
	datos->canales[id]->usuarios.push_back(nickname);
	datos->canales[id]->umodes.push_back('x');
	chan_mute.unlock();
}

void Data::DelUsersToChan(int id, int idn) {
	chan_mute.lock();
	datos->canales[id]->usuarios.erase(datos->canales[id]->usuarios.begin() + idn);
	datos->canales[id]->umodes.erase(datos->canales[id]->umodes.begin() + idn);
	if (datos->canales[id]->usuarios.size() == 0)
		datos->canales.erase(datos->canales.begin() + id);
	chan_mute.unlock();
}

int Data::GetChanPosition (string canal) {
	for (unsigned int i = 0; i < datos->canales.size(); i++)
		if (mayus(datos->canales[i]->nombre) == mayus(canal))
			return i;
	return -1;
}

int Data::GetNickPosition (string canal, string nickname) {
	int id = GetChanPosition(canal);
	if (id < 0)
		return -1;
	for (unsigned int i = 0; i < datos->canales[id]->usuarios.size(); i++)
		if (mayus(datos->canales[id]->usuarios[i]) == mayus(nickname))
			return i;
	return -1;
}

void Data::SetOper (string nickname) {
	Oper *oper = new Oper();
		oper->nickoper = nickname;
		oper->tiene_o = false;
	oper_mute.lock();
	datos->operadores.push_back(oper);
	oper_mute.unlock();
}

void Data::SNICK(string nickname, string ip, string cloakip, long int creado, string nodo) {
	Nick *nickinfo = new Nick();
		nickinfo->stream = NULL;
		nickinfo->nickname = nickname;
		nickinfo->ident = "ZeusiRCd";
		nickinfo->login = creado;
		nickinfo->nodo = nodo;
		nickinfo->conectado = true;
		nickinfo->cloakip = cloakip;
		nickinfo->ip = ip;
	nick_mute.lock();
	datos->nicks.push_back(nickinfo);
	nick_mute.unlock();
	return;
}

bool Data::Match(const char *first, const char *second)
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

int Data::GetUsuarios () {
	size_t usuarios = datos->nicks.size();
	return static_cast<int>(usuarios);
}

int Data::GetCanales () {
	size_t canales = datos->canales.size();
	return static_cast<int>(canales);
}

int Data::GetOperadores () {
	size_t opers = datos->operadores.size();
	return static_cast<int>(opers);
}

int Data::GetServidores () {
	size_t servidores = datos->servers.size();
	return static_cast<int>(servidores);
}
