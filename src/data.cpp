#include "include.h"
#include "sha256.h"

using namespace std;

Data *datos = new Data();

mutex sock_mute, nick_mute, chan_mute, server_mute, oper_mute, memo_mute;

void Data::CrearNick(TCPStream *stream, string _nick) {
	Nick *nickinfo = new Nick();
		nickinfo->stream = stream;
		nickinfo->nickname = _nick;
		nickinfo->ident = "ZeusiRCd";
		nickinfo->login = static_cast<long int> (time(NULL));
		nickinfo->nodo = config->Getvalue("serverName");
		nickinfo->conectado = true;
		if (stream->cgiirc.length() > 0) {
			nickinfo->ip = stream->cgiirc;
			nickinfo->tiene_w = false;
		} else
			nickinfo->ip = stream->getPeerIP();
			
	nickinfo->cloakip = sha256(nickinfo->ip).substr(0, 16);
	std::lock_guard<std::mutex> lock(nick_mute);
	datos->nicks.push_back(nickinfo);
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
			std::lock_guard<std::mutex> lock(oper_mute);
			datos->operadores.erase(datos->operadores.begin() + i);
		}
}

void Data::BorrarNick(TCPStream *stream) {
	int id = datos->BuscarIDStream(stream);
	if (id < 0)
		return;
	if (oper->IsOper(datos->nicks[id]->nickname) == 1)
		datos->DelOper(datos->nicks[id]->nickname);
	std::lock_guard<std::mutex> lock(nick_mute);
	datos->nicks.erase(datos->nicks.begin() + id);
	return;
}

void Data::BorrarNickByNick(string nickname) {
	int id = datos->BuscarIDNick(nickname);
	if (id < 0)
		return;
	if (oper->IsOper(nickname) == 1)
		datos->DelOper(nickname);
	std::lock_guard<std::mutex> lock(nick_mute);
	datos->nicks.erase(datos->nicks.begin() + id);
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
	std::lock_guard<std::mutex> lock(server_mute);
	datos->servers.push_back(servidor);
}

void Data::DeleteServer (string name) {
	int id = 0;
	for (unsigned int i = 0; i < datos->servers.size(); i++) {
		if (datos->servers[i]->nombre == name) {
			id = i;
			break;
		}
	}
	std::lock_guard<std::mutex> lock(server_mute);
	datos->servers.erase(datos->servers.begin() + id);
}

void Data::Conexiones(string principal, string linkado) {
	std::lock_guard<std::mutex> lock(server_mute);
	for (unsigned int i = 0; i < datos->servers.size(); i++)
		if (principal == datos->servers[i]->nombre) {
			datos->servers[i]->connected.push_back(linkado);
		}
}

void Data::CrearCanal(string nombre) {
	Chan *canal = new Chan();
		canal->nombre = nombre;
		canal->creado = static_cast<long int> (time(NULL));
	std::lock_guard<std::mutex> lock(chan_mute);
	datos->canales.push_back(canal);
}

void Data::AddUsersToChan(int id, string nickname) {
	User *data = new User();
	data->nombre = nickname;
	data->modo = 'x';
	std::lock_guard<std::mutex> lock(chan_mute);
	datos->canales[id]->usuarios.push_back(data);
}

void Data::DelUsersToChan(int id, int idn) {
	chan_mute.lock();
	datos->canales[id]->usuarios.erase(datos->canales[id]->usuarios.begin() + idn);
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
		if (mayus(datos->canales[id]->usuarios[i]->nombre) == mayus(nickname))
			return i;
	return -1;
}

void Data::InsertMemo (string sender, string receptor, long int fecha, string mensa) {
	Memo *mensaje = new Memo();
		mensaje->sender = sender;
		mensaje->receptor = receptor;
		mensaje->time = fecha;
		mensaje->mensaje = mensa;
	std::lock_guard<std::mutex> lock(memo_mute);
	datos->memos.push_back(mensaje);
	return;
}

void Data::DeleteMemos (string receptor) {
	std::lock_guard<std::mutex> lock(memo_mute);
	for (int i = datos->memos.size() -1; i > -1; i--)
		if (mayus(datos->memos[i]->receptor) == mayus(receptor))
			datos->memos.erase(datos->memos.begin() + i);
}

void Data::SetOper (string nickname) {
	Oper *oper = new Oper();
		oper->nickoper = nickname;
	std::lock_guard<std::mutex> lock(oper_mute);
	datos->operadores.push_back(oper);
}

void Data::SNICK(string nickname, string ip, string cloakip, long int creado, string nodo, string modos) {
	Nick *nickinfo = new Nick();
		nickinfo->stream = NULL;
		nickinfo->nickname = nickname;
		nickinfo->ident = "ZeusiRCd";
		nickinfo->login = creado;
		nickinfo->nodo = nodo;
		nickinfo->conectado = true;
		nickinfo->cloakip = cloakip;
		nickinfo->ip = ip;
		for (unsigned int i = 1; i < modos.length(); i++) {
			if (modos[i] == 'r')
				nickinfo->tiene_r = true;
			else if (modos[i] == 'z')
				nickinfo->tiene_z = true;
			else if (modos[i] == 'o')
				nickinfo->tiene_o = true;
			else if (modos[i] == 'w')
				nickinfo->tiene_w = true;
		}
	std::lock_guard<std::mutex> lock(nick_mute);
	datos->nicks.push_back(nickinfo);
	return;
}

void Data::ChannelBan(string who, string mascara, string canal) {
	int id = datos->GetChanPosition(canal);
	Ban *ban = new Ban();
		ban->mascara = mascara;
		ban->who = who;
		ban->fecha = static_cast<long int> (time(NULL));
	datos->canales[id]->bans.push_back(ban);
}

void Data::UnBan(string mascara, string canal) {
	int id = datos->GetChanPosition(canal);
	for (unsigned int i = 0; i < datos->canales[id]->bans.size(); i++)
		if (mascara == datos->canales[id]->bans[i]->mascara)
			datos->canales[id]->bans.erase(datos->canales[id]->bans.begin() + i);
}

string Data::Time(long int tiempo) {
	int dias, horas, minutos, segundos = 0;
	string total;
	long int now = static_cast<long int> (time(NULL));
	tiempo = now - tiempo;
	if (tiempo <= 0)
		return "0s";
	if ((dias = tiempo / 86400) > 1)
		tiempo = tiempo % 86400;
	if ((horas = tiempo / 3600) > 1)
		tiempo = tiempo % 3600;
	if ((minutos = tiempo / 60) > 1)
		tiempo = tiempo % 60;
	segundos = tiempo;
	
	if (dias) {
		total.append(to_string(dias));
		total.append("d ");
	} if (horas) {
		total.append(to_string(horas));
		total.append("h ");
	} if (minutos) {
		total.append(to_string(minutos));
		total.append("m ");
	} if (segundos) {
		total.append(to_string(segundos));
		total.append("s");
	}
	return total;
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
