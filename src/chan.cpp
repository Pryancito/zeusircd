#include "include.h"

Chan *chan = new Chan();

using namespace std;

bool Chan::IsInChan(string canal, string nickname) {
	if (datos->GetNickPosition(canal, nickname) >= 0)
		return 1;
	else
		return 0;
}

void Chan::Join(string canal, string nickname) {
	int id = datos->GetChanPosition(canal);
	if (id < 0) {
		datos->CrearCanal(canal);
		id = datos->GetChanPosition(canal);
	}
	datos->AddUsersToChan(id, nickname);
	return;
}

void Chan::Part(string canal, string nickname) {
	int id = datos->GetChanPosition(canal);
	if (id < 0)
		return;
	int idn = datos->GetNickPosition(canal, nickname);
	if (idn < 0)
		return;

	datos->DelUsersToChan(id, idn);
	return;
}

void Chan::PropagateJoin(string canal, int sID) {
	TCPStream *stream;
	int id = datos->GetChanPosition(canal);
	if (id < 0)
		return;
	if (sID < 0)
		return;
	lock_guard<std::mutex> lock(nick_mute);
	for (unsigned int i = 0; i < datos->canales[id]->usuarios.size(); i++) {
		stream = datos->BuscarStream(datos->canales[id]->usuarios[i]);
		if (stream == NULL)
			continue;
		sock->Write(stream, ":" + nick->FullNick(sID) + " JOIN :" + datos->canales[id]->nombre + "\r\n");
	}
	return;
}

void Chan::PropagatePart(string canal, string nickname) {
	int id = datos->GetChanPosition(canal);
	if (id < 0)
		return;
	lock_guard<std::mutex> lock(nick_mute);
	for (unsigned int i = 0; i < datos->canales[id]->usuarios.size(); i++) {
		TCPStream *stream = datos->BuscarStream(datos->canales[id]->usuarios[i]);
		if (stream == NULL)
			continue;
		sock->Write(stream, ":" + nick->FullNick(datos->BuscarIDNick(nickname)) + " PART " + datos->canales[id]->nombre + "\r\n");
	}
	return;
}

string Chan::GetRealName(string canal) {
	int id = datos->GetChanPosition(canal);
	if (id < 0)
		return "";
	return datos->canales[id]->nombre;
}

void Chan::SendNAMES(TCPStream *stream, string canal) {
	string nickname = nick->GetNick(datos->BuscarIDStream(stream)), nicks = "";
	int id = datos->GetChanPosition(canal);
	if (id < 0)
		return;
	lock_guard<std::mutex> lock(nick_mute);
	for (unsigned int i = 0; i < datos->canales[id]->usuarios.size(); i++) {
		for (unsigned int j = 0; j < 20 && i < datos->canales[id]->usuarios.size(); j++, i++) {
			if (nicks.length() > 0)
				nicks.append(" ");
			if (datos->canales[id]->umodes[i] == 'o')
				nicks.append("@");
			else if (datos->canales[id]->umodes[i] == 'h')
				nicks.append("%");
			else if (datos->canales[id]->umodes[i] == 'v' )
				nicks.append("+");
			nicks.append(datos->canales[id]->usuarios[i]);
		}
		if (stream != NULL)
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 353 " + nickname + " = " + datos->canales[id]->nombre + " :" + nicks + "\r\n");
		nicks.clear();
	}
	if (stream != NULL)
	sock->Write(stream, ":" + config->Getvalue("serverName") + " 366 " + nickname + " " + datos->canales[id]->nombre + " :End of /NAMES list\r\n");
}

void Chan::PropagarNick(string viejo, string nuevo) {
	lock_guard<std::mutex> lock(nick_mute);
	for (unsigned int i = 0; i < datos->canales.size(); i++) {
		if (IsInChan(datos->canales[i]->nombre, viejo) == 1) {
			for (unsigned int j = 0; j < datos->canales[i]->usuarios.size(); j++) {
				if (mayus(viejo) != mayus(datos->canales[i]->usuarios[j])) {
					TCPStream *nickstream = datos->BuscarStream(datos->canales[i]->usuarios[j]);
					if (nickstream != NULL && datos->BuscarIDNick(viejo) >= 0)
						sock->Write(nickstream, ":" + nick->FullNick(datos->BuscarIDNick(viejo)) + " NICK :" + nuevo + "\r\n");
				}
			}
			int id = datos->GetNickPosition(datos->canales[i]->nombre, viejo);
			datos->canales[i]->usuarios[id] = nuevo;
		}
	}
}

void Chan::PropagarQUIT(TCPStream *stream) {
	int id = datos->BuscarIDStream(stream);
	string nickname = nick->GetNick(id);
	lock_guard<std::mutex> lock(nick_mute);
	for (unsigned int i = 0; i < datos->canales.size(); i++) {
		if (IsInChan(datos->canales[i]->nombre, nickname) == 1) {
			for (unsigned int j = 0; j < datos->canales[i]->usuarios.size(); j++) {
				TCPStream *send = datos->BuscarStream(datos->canales[i]->usuarios[j]);
				if (mayus(nickname) == mayus(datos->canales[i]->usuarios[j]))
					continue;
				if (send != NULL)
					sock->Write(send, ":" + nick->FullNick(id) + " QUIT :QUIT" + "\r\n");
			}
			Part(datos->canales[i]->nombre, nickname);
		}
	}
	return;
}

void Chan::PropagarQUITByNick(string nickname) {
	int id = datos->BuscarIDNick(nickname);
	lock_guard<std::mutex> lock(nick_mute);
	for (unsigned int i = 0; i < datos->canales.size(); i++) {
		if (IsInChan(datos->canales[i]->nombre, nickname) == 1) {
			for (unsigned int j = 0; j < datos->canales[i]->usuarios.size(); j++) {
				TCPStream *send = datos->BuscarStream(datos->canales[i]->usuarios[j]);
				if (mayus(nickname) == mayus(datos->canales[i]->usuarios[j]))
					continue;
				if (send != NULL)
					sock->Write(send, ":" + nick->FullNick(id) + " QUIT :QUIT" + "\r\n");
			}
			Part(datos->canales[i]->nombre, nickname);
		}
	}
	return;
}

void Chan::PropagarMSG(string nickname, string chan, string mensaje) {
	int id = datos->GetChanPosition(chan);
	if (id < 0)
		return;
	lock_guard<std::mutex> lock(nick_mute);
	for (unsigned int i = 0; i < datos->canales[id]->usuarios.size(); i++) {
		TCPStream *nickstream = datos->BuscarStream(datos->canales[id]->usuarios[i]);
		if (mayus(nickname) == mayus(datos->canales[id]->usuarios[i]))
			continue;
		if (nickstream != NULL)
			sock->Write(nickstream, ":" + nick->FullNick(datos->BuscarIDNick(nickname)) + " " + mensaje);
	}
	return;
}

void Chan::PropagarMODE(string who, string nickname, string chan, char modo, bool add) {
	int id = datos->GetChanPosition(chan);
	if (id < 0)
		return;
	char simbol;
	lock_guard<std::mutex> lock(nick_mute);
	for (unsigned int i = 0; i < datos->canales[id]->usuarios.size(); i++) {
			if (add == 1) {
				datos->canales[id]->umodes[i] = modo;
				simbol = '+';
			} else {
				datos->canales[id]->umodes[i] = 'x';
				simbol = '-';
			}
			TCPStream *nickstream = datos->BuscarStream(datos->canales[id]->usuarios[i]);
			if (nickstream != NULL)
			sock->Write(nickstream, ":" + who + " MODE " + chan + " " + simbol + modo + " " + nickname + "\r\n");
	}
	return;
}

void Chan::Lista (std::string canal, TCPStream *stream) {
	string nickname = nick->GetNick(datos->BuscarIDStream(stream));
	sock->Write(stream, ":" + config->Getvalue("serverName") + " 321 " + nickname + " Channel :Lista de canales." + "\r\n");
	
	for (unsigned int i = 0; i < datos->canales.size(); i++) {
		if (datos->Match(canal.c_str(), datos->canales[i]->nombre.c_str()) == 1) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 322 " + nickname + " " + datos->canales[i]->nombre + " " + to_string(datos->canales[i]->usuarios.size()) + " :" + "\r\n");
		}
	}
	
	sock->Write(stream, ":" + config->Getvalue("serverName") + " 323 " + nickname + " :Fin de /LIST." + "\r\n");
	return;
}

int Chan::MaxChannels(string nickname) {
	int chan = 0;
	for (unsigned int i = 0; i < datos->canales.size(); i++)
		if (IsInChan(datos->canales[i]->nombre, nickname) == 1)
			chan++;
	return chan;
}
