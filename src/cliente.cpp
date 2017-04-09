#include "include.h"
#include "sha256.h"
#include <algorithm>
#include <vector>
#include <locale>

using namespace std;

Cliente *cliente = new Cliente();

std::vector<std::string> split(const std::string &str, int delimiter(int) = ::isspace){
  vector<string> result;
  auto e=str.end();
  auto i=str.begin();
  while(i!=e){
    i=find_if_not(i,e, delimiter);
    if(i==e) break;
    auto j=find_if(i,e, delimiter);
    result.push_back(string(i,j));
    i=j;
  }
  return result;
}

std::vector<std::string> split_nick(const std::string &str){
	vector <string> tokens;
	int kill = 0;
	string buf;
	for (unsigned int i = 0; i < str.length(); i++) {
		if (str[i] == ':' || str[i] == '!') {
			if (str[i] == '!')
				kill = 1;
			if (!buf.empty()) {
				tokens.push_back(buf);
				buf.clear();
			}
		} else {
			buf.append(str.substr(i, 1));
		}
	} tokens.push_back(buf); tokens.push_back(to_string(kill));
	return tokens;
}

bool checknick (string nick) {
	std::locale loc(config->Getvalue("locale").c_str());
	for (unsigned int i = 0; i < nick.length(); i++)
		if (!isalnum(nick[i], loc) && nick[i] != '_')
			return false;
	return true;
}

bool checkchan (const string chan) {
	std::locale loc(config->Getvalue("locale").c_str());
	for (unsigned int i = 1; i < chan.length(); i++)
		if ((!isalnum(chan[i], loc) && chan[i] != '_') || chan[0] != '#')
			return false;
	return true;
}

void mayuscula (string &str) {
	for (unsigned int i = 0; i < str.length(); i++)
		str[i] = toupper(str[i]);
}

string mayus (const string str) {
	std::locale loc(config->Getvalue("locale").c_str());
	string buf = str;
	for (unsigned int i = 0; i < str.length(); i++) {
		wchar_t c = str[i];
		buf[i] = toupper(c, loc);
	}
	return buf;
}

bool Cliente::ProcesaMensaje (TCPStream* stream, string mensaje) {
	if (mensaje.length() == 0 || mensaje == "\r\n" || mensaje == "\r" || mensaje == "\n")
		return 0;
	vector<string> x = split(mensaje);
	string cmd = x[0];
	mayuscula(cmd);
	int sID = datos->BuscarIDStream(stream);
	string nickname, pass;
	int kill = 0;
	if (cmd == "NICK") {
		if (x.size() < 2) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 431 :No has proporcionado un Nick." + "\r\n");
			return 0;
		}
		vector <string> nick2 = split_nick(x[1]);
		if (nick2.size() == 2) {
			nickname = nick2[0];
			pass = nick2[1];
		} else if (nick2.size() == 3) {
			nickname = nick2[0];
			pass = nick2[1];
			kill = stoi(nick2[2]);
		} else {
			nickname = x[1];
			pass = "";
		}
		if (checknick(nickname) == false) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 432 :El Nick contiene caracteres no validos." + "\r\n");
			return 0;
		} else if (nickname.length() > (unsigned int )stoi(config->Getvalue("nicklen"))) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 432 :El Nick es demasiado largo." + "\r\n");
			return 0;
		} else if (mayus(nick->GetNick(sID)) == mayus(nickname)) {
			sock->Write(stream, ":" + nick->FullNick(sID) + " NICK " + nickname + "\r\n");
			chan->PropagarNick(nick->GetNick(sID), nickname);
			server->SendToAllServers("SVSNICK " + nick->GetNick(sID) + " " + nickname);
			nick->CambioDeNick(sID, nickname);
			return 0;
		} else if (nick->Existe(nickname) == true && kill == 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 433 :El Nick " + nickname + " esta en uso." + "\r\n");
			return 0;
		} else if (sID < 0) {
			if (nickserv->IsRegistered(nickname) == 1) {
				if (pass == "") {
					sock->Write(stream, ":NiCK!*@* NOTICE " + nickname + " :No has proporcionado una password." + "\r\n");
					return 0;
				} else if (nickserv->Login(nickname, pass) == 0) {
					sock->Write(stream, ":NiCK!*@* NOTICE " + nickname + " :La password es incorrecta." + "\r\n");
					return 0;
				} else {
					if (kill == 1 && nick->Existe(nickname) == true) {
						TCPStream *nickstream = datos->BuscarStream(nickname);
						chan->PropagarQUIT(nickstream);
						server->SendToAllServers("QUIT " + nickname);
						datos->BorrarNick(nickstream);
						close(nickstream->getPeerSocket());
					}
					sock->Write(stream, ":NiCK!*@* NOTICE " + nickname + " :Bienvenido a casa." + "\r\n");
					datos->CrearNick(stream, nickname);
					Bienvenida(stream, nickname);
					int sID = datos->BuscarIDNick(nickname);
					string modos = "+";
					if (stream->getSSL() == 1) {
						datos->nicks[sID]->tiene_z = true;
						sock->Write(stream, ":" + config->Getvalue("serverName") + " MODE " + nickname + " +z\r\n");
						modos.append("z");
					}
					if (stream->cgiirc.length() > 0) {
						datos->nicks[sID]->tiene_w = true;
						sock->Write(stream, ":" + config->Getvalue("serverName") + " MODE " + nickname + " +w\r\n");
						modos.append("w");
					}
					modos.append("r");
					server->SendToAllServers("SNICK " + datos->nicks[sID]->nickname + " " + datos->nicks[sID]->ip + " " + datos->nicks[sID]->cloakip + " " + to_string(datos->nicks[sID]->login) + " " + datos->nicks[sID]->nodo + " " + modos);
					if (datos->nicks[sID]->tiene_r == false) {
						sock->Write(stream, ":" + config->Getvalue("serverName") + " MODE " + nickname + " +r\r\n");
						datos->nicks[sID]->tiene_r = true;
					}
					return 0;
				}
			} else {
				datos->CrearNick(stream, nickname);
				Bienvenida(stream, nickname);
				int sID = datos->BuscarIDNick(nickname);
				string modos = "+";
				if (stream->getSSL() == 1) {
					datos->nicks[sID]->tiene_z = true;
					sock->Write(stream, ":" + config->Getvalue("serverName") + " MODE " + nickname + " +z\r\n");
					modos.append("z");
				}
				if (stream->cgiirc.length() > 0) {
						datos->nicks[sID]->tiene_w = true;
						sock->Write(stream, ":" + config->Getvalue("serverName") + " MODE " + nickname + " +w\r\n");
						modos.append("w");
				}
				server->SendToAllServers("SNICK " + datos->nicks[sID]->nickname + " " + datos->nicks[sID]->ip + " " + datos->nicks[sID]->cloakip + " " + to_string(datos->nicks[sID]->login) + " " + datos->nicks[sID]->nodo + " " + modos);
				return 0;
			}
		} else {
			if (nickserv->IsRegistered(nickname) == 1) {
				if (pass == "") {
					sock->Write(stream, ":NiCK!*@* NOTICE " + nickname + " :No has proporcionado una password." + "\r\n");
					return 0;
				} else if (nickserv->Login(nickname, pass) == 0) {
					sock->Write(stream, ":NiCK!*@* NOTICE " + nickname + " :La password es incorrecta." + "\r\n");
					return 0;
				} else {
					if (kill == 1 && nick->Existe(nickname) == true) {
						TCPStream *nickstream = datos->BuscarStream(nickname);
						chan->PropagarQUIT(nickstream);
						server->SendToAllServers("QUIT " + nickname);
						datos->BorrarNick(nickstream);
						close(nickstream->getPeerSocket());
					}
					sock->Write(stream, ":NiCK!*@* NOTICE " + nickname + " :Bienvenido a casa." + "\r\n");
					sock->Write(stream, ":" + nick->FullNick(sID) + " NICK " + nickname + "\r\n");
					chan->PropagarNick(nick->GetNick(sID), nickname);
					server->SendToAllServers("SVSNICK " + nick->GetNick(sID) + " " + nickname);
					nick->CambioDeNick(sID, nickname);
					if (datos->nicks[sID]->tiene_r == false) {
						sock->Write(stream, ":" + config->Getvalue("serverName") + " MODE " + nickname + " +r\r\n");
						datos->nicks[sID]->tiene_r = true;
					}
					return 0;
				}
			} else {
				sock->Write(stream, ":" + nick->FullNick(sID) + " NICK " + nickname + "\r\n");
				chan->PropagarNick(nick->GetNick(sID), nickname);
				server->SendToAllServers("SVSNICK " + nick->GetNick(sID) + " " + nickname);
				nick->CambioDeNick(sID, nickname);
				if (datos->nicks[sID]->tiene_r == true) {
					sock->Write(stream, ":" + config->Getvalue("serverName") + " MODE " + nickname + " -r\r\n");
					datos->nicks[sID]->tiene_r = false;
				}
				return 0;
			}
		}
	} else if (cmd == "USER") {
		if (x.size() < 2) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :Necesito mas datos." + "\r\n");
			return 0;
		} else if (checknick(x[1]) == false) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :Tu ident contiene caracteres no validos." + "\r\n");
			return 0;
		} else if (sID < 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return 0;
		} else if (nick->Registrado(sID) == 1) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 462 :Ya estas registrado." + "\r\n");
			return 0;
		} else if (nick->Registrado(sID) == 0 && nick->Conectado(sID) == 1){
			nick->SetIdent(sID, x[1]);
			server->SendToAllServers("SUSER " + nick->GetNick(sID) + " " + x[1]);
			return 0;
		} else return 0;
	} else if (cmd == "QUIT") {
		return 1;
	} else if (cmd == "JOIN") {
		if (x.size() < 2) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :Necesito mas datos." + "\r\n");
			return 0;
		} else if (checkchan(x[1]) == false) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :El Canal contiene caracteres no validos." + "\r\n");
			return 0;
		} else if (x[1].length() > (unsigned int )stoi(config->Getvalue("chanlen"))) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :El Canal es demasiado largo." + "\r\n");
			return 0;
		} else if (sID < 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return 0;
		} else if (chan->IsInChan(x[1], nick->GetNick(sID)) == 1) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :Ya estas dentro del canal." + "\r\n");
			return 0;
		} else if (chan->MaxChannels(nick->GetNick(sID)) >= stoi(config->Getvalue("maxchannels"))) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :Has entrado en demasiados canales." + "\r\n");
			return 0;
		} else if (chan->IsBanned(sID, x[1]) == 1) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :Estas baneado, no puedes entrar al canal." + "\r\n");
			return 0;
		} else {
			string mascara = nick->GetNick(sID) + "!" + nick->GetIdent(sID) + "@" + nick->GetCloakIP(sID);
			if (chanserv->IsAKICK(mascara, x[1]) == 1 && oper->IsOper(nick->GetNick(sID)) == 0) {
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :Tienes AKICK en este canal, no puedes entrar." + "\r\n");
				return 0;
			}
			mascara = nick->GetNick(sID) + "!" + nick->GetIdent(sID) + "@" + nick->GetvHost(sID);
			if (chanserv->IsAKICK(mascara, x[1]) == 1 && oper->IsOper(nick->GetNick(sID)) == 0) {
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :Tienes AKICK en este canal, no puedes entrar." + "\r\n");
				return 0;
			}
			chan->Join(x[1], nick->GetNick(sID));
			chan->PropagateJoin(x[1], sID);
			chan->SendNAMES(stream, x[1]);
			
			int i = datos->GetChanPosition(x[1]);
			int j = datos->GetNickPosition(x[1], nick->GetNick(sID));
			if (chanserv->IsRegistered(x[1]) == 1) {
				string sql = "SELECT REGISTERED from CANALES WHERE NOMBRE='" + x[1] + "' COLLATE NOCASE;";
				int registrado = db->SQLiteReturnInt(sql);
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 329 " + nick->GetNick(sID) + " " + x[1] + " " + to_string(registrado) + "\r\n");
				sql = "SELECT TOPIC from CANALES WHERE NOMBRE='" + x[1] + "' COLLATE NOCASE;";
				string topic = db->SQLiteReturnString(sql);
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 332 " + nick->GetNick(sID) + " " + x[1] + " :" + topic + "\r\n");
				if (datos->canales[i]->tiene_r == false) {
					chan->PropagarMODE("CHaN!*@*", "", x[1], 'r', 1);
					datos->canales[i]->tiene_r = true;
				}
				chanserv->CheckModes(nick->GetNick(sID), x[1]);
			}
			server->SendToAllServers("SJOIN " + nick->GetNick(sID) + " " + x[1] + " +" + datos->canales[i]->usuarios[j]->nombre);	
			return 0;
		}
	} else if (cmd == "PART") {
		if (x.size() < 2) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :Necesito mas datos." + "\r\n");
			return 0;
		} else if (checkchan(x[1]) == false) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :El Canal contiene caracteres no validos." + "\r\n");
			return 0;
		} else if (sID < 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return 0;
		} else if (chan->IsInChan(x[1], nick->GetNick(sID)) == 1) {
			chan->PropagatePart(x[1], nick->GetNick(sID));
			chan->Part(x[1], nick->GetNick(sID));
			server->SendToAllServers("SPART " + nick->GetNick(sID) + " " + x[1] + "\r\n");
			return 0;
		} else
			return 0;
	} else if (cmd == "PRIVMSG" || cmd == "NOTICE") {
		if (x.size() < 2) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 411 :Necesito un destino." + "\r\n");
			return 0;
		} else if (x.size() < 3) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 412 :Necesito un texto." + "\r\n");
			return 0;
		} else if (sID < 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return 0;
		} else if (x[1][0] == '#') {
			if (chan->IsInChan(x[1], nick->GetNick(sID)) == 0 || chan->IsBanned(sID, x[1]) == 1) {
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 404 :No puedes enviar texto al canal." + "\r\n");
				return 0;
			} else {
				if (strstr(mensaje.c_str(), "\r\n") == NULL)
					mensaje.append("\r\n");
				chan->PropagarMSG(nick->GetNick(sID), x[1], mensaje);
				server->SendToAllServers(nick->GetNick(sID) + " " + mensaje);
				return 0;
			}
		} else {
			if (datos->BuscarIDNick(x[1]) < 0) {
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 401 :El Nick no existe." + "\r\n");
				return 0;
			} else if (nickserv->GetOption("ONLYREG", x[1]) == 1 && nickserv->IsRegistered(nick->GetNick(sID)) == 0) {
				sock->Write(stream, ":NiCK!*@* NOTICE " + nick->GetNick(sID) + " :El nick solo admite privados de usuarios registrados." + "\r\n");
				return 0;
			} else {
				if (strstr(mensaje.c_str(), "\r\n") == NULL)
					mensaje.append("\r\n");
				TCPStream *nickstream = datos->BuscarStream(x[1]);
				if (nickstream == NULL)
					server->SendToAllServers(nick->GetNick(sID) + " " + mensaje);
				else
					sock->Write(datos->BuscarStream(x[1]), ":" + nick->FullNick(sID) + " " + mensaje);
				return 0;
			}
		}
	} else if (cmd == "LIST") {
		if (sID < 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return 0;
		} else if (x.size() > 1) {
			chan->Lista(x[1], stream);
			return 0;
		} else {
			string lista = "*";
			chan->Lista(lista, stream);
			return 0;
		}
	} else if (cmd == "VERSION") {
		if (sID < 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return 0;
		} else if (oper->IsOper(nick->GetNick(sID)) == 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :No tienes privilegios de iRCop." + "\r\n");
			return 0;
		} else if (oper->IsOper(nick->GetNick(sID)) == 1) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :La ultima version de BDD es: " + db->GetLastRecord() + "\r\n");
			return 0;
		} else return 0;
	} else if (cmd == "OPER") {
		if (x.size() < 3) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :Necesito mas datos." + "\r\n");
			return 0;
		} else if (sID < 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return 0;
		} else if (checknick(x[1]) == false) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 432 :El Nick contiene caracteres no validos." + "\r\n");
			return 0;
		} else if (oper->IsOper(nick->GetNick(datos->BuscarIDStream(stream))) == 1) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 381 :Ya eres iRCop." + "\r\n");
			return 0;
		} else if (oper->Login(nick->GetNick(sID), x[1], x[2]) == 1) {
			server->SendToAllServers("SOPER " + nick->GetNick(sID));
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :Has sido identificado como iRCop." + "\r\n");
			return 0;
		} else {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 491 :Identificacion fallida." + "\r\n");
			return 0;
		}
	} else if (cmd == "REHASH") {
		if (sID < 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return 0;
		} else if (oper->IsOper(nick->GetNick(sID)) == 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :No tienes privilegios de iRCop." + "\r\n");
			return 0;
		} else if (oper->IsOper(nick->GetNick(sID)) == 1) {
			config->Cargar();
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :La configuracion ha sido recargada." + "\r\n");
			return 0;
		} else return 0;
	} else if (cmd == "KICK") {
		if (x.size() < 4) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :Necesito mas datos." + "\r\n");
			return 0;
		} else if (sID < 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return 0;
		} else if (chan->IsInChan(x[1], nick->GetNick(sID)) == 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :No estas dentro del canal." + "\r\n");
			return 0;
		} else if (chan->IsInChan(x[1], x[2]) == 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :El nick no esta dentro del canal." + "\r\n");
			return 0;
		} else if (datos->BuscarIDNick(x[2]) < 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :El nick no existe." + "\r\n");
			return 0;
		} else {
			int id = datos->GetChanPosition(x[1]);
			int uid = datos->GetNickPosition(x[1], nick->GetNick(sID));
			if (datos->canales[id]->usuarios[uid]->modo != 'o' && datos->canales[id]->usuarios[uid]->modo != 'h') {
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :No tienes status de operador (+o) ni de halfop (+h)." + "\r\n");
				return 0;
			}
			int posicion = 4 + cmd.length() + x[1].length() + x[2].length();
			string motivo = mensaje.substr(posicion);
			string objetivo = nick->GetNick(datos->BuscarIDNick(x[2]));
			chan->PropagateKICK(sID, x[1], objetivo, motivo);
			chan->Part(x[1], objetivo);
			server->SendToAllServers("SKICK " + nick->GetNick(sID) + " " + x[1] + " " + objetivo + " :" + motivo + "\r\n");
			return 0;
		}
	} else if (cmd == "MKPASSWD") {
		if (x.size() < 2) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :Necesito mas datos." + "\r\n");
			return 0;	
		} else if (sID < 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return 0;
		} else if (oper->IsOper(nick->GetNick(sID)) == 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :No tienes privilegios de iRCop." + "\r\n");
			return 0;
		} else if (oper->IsOper(nick->GetNick(sID)) == 1) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :La password " + x[1] + " encriptada es: " + sha256(x[1]) + "\r\n");
			return 0;
		} else return 0;
	} else if (cmd == "STATS") {
		if (sID < 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return 0;
		} else {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 " + nick->GetNick(sID) + " :Hay \002" + to_string(datos->GetUsuarios()) + "\002 usuarios y \002" + to_string(datos->GetCanales()) + "\002 canales.\r\n");
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 " + nick->GetNick(sID) + " :Hay \002" + to_string(nickserv->GetNicks()) + "\002 nicks registrados y \002" + to_string(chanserv->GetChans()) + "\002 canales registrados.\r\n");
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 " + nick->GetNick(sID) + " :Hay \002" + to_string(datos->GetOperadores()) + "\002 iRCops conectados." + "\r\n");
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 " + nick->GetNick(sID) + " :Hay \002" + to_string(datos->GetServidores()) + "\002 servidores conectados." + "\r\n");
			return 0;
		}
	} else if (cmd == "SERVERS") {
		if (sID < 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return 0;
		} else if (oper->IsOper(nick->GetNick(sID)) == 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :No tienes privilegios de iRCop." + "\r\n");
			return 0;
		} else if (oper->IsOper(nick->GetNick(sID)) == 1) {
			server->ListServers(stream);
			return 0;
		} else return 0;
	} else if (cmd == "CONNECT") {
		if (x.size() < 2) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :Necesito mas datos." + "\r\n");
			return 0;
		} else if (oper->IsOper(nick->GetNick(sID)) == 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :No tienes privilegios de iRCop." + "\r\n");
			return 0;
		} else if (server->IsAServer(x[1]) == 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :El servidor no esta en mi lista." + "\r\n");
			return 0;
		} else if (server->IsConected(x[1]) == 1) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :El servidor ya esta conectado." + "\r\n");
			return 0;
		} else {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :Conectando..." + "\r\n");
			server->Conectar(x[1]);
			return 0;
		}
	} else if (cmd == "SQUIT") {
		if (x.size() < 2) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :Necesito mas datos." + "\r\n");
			return 0;
		} else if (oper->IsOper(nick->GetNick(sID)) == 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :No tienes privilegios de iRCop." + "\r\n");
			return 0;
		} else if (server->IsAServer(x[1]) == 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :El servidor no esta en mi lista." + "\r\n");
			return 0;
		} else if (server->IsConected(x[1]) == 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :El servidor no esta conectado." + "\r\n");
			return 0;
		} else if (server->IsConected(x[1]) == 1) {
			for (unsigned int i = 0; i < datos->servers.size(); i++)
				if (datos->servers[i]->nombre == server->FindName(x[1])) {
					for (unsigned int j = 0; j < datos->servers[i]->connected.size(); j++) {
						server->SQUITByServer(datos->servers[i]->connected[j]);
						datos->DeleteServer(datos->servers[i]->connected[j]);
					}
					server->SQUITByServer(server->FindName(x[1]));
					shutdown(datos->servers[i]->stream->getPeerSocket(), 2);
					datos->DeleteServer(server->FindName(x[1]));
				}
			server->SendToAllServers("SQUIT " + server->FindName(x[1]));
			return 0;
		}
	} else if (cmd == "WHOIS") {
		if (x.size() < 2) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 401 " + nick->GetNick(sID) + " " + x[1] + " :Necesito mas datos." + "\r\n");
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 318 " + nick->GetNick(sID) + " " + x[1] + " :Fin de /WHOIS." + "\r\n");
			return 0;
		} else if (sID < 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 401 " + nick->GetNick(sID) + " " + x[1] + " :No te has registrado." + "\r\n");
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 318 " + nick->GetNick(sID) + " " + x[1] + " :Fin de /WHOIS." + "\r\n");
			return 0;
		} else {
			int wid = datos->BuscarIDNick(x[1]);
			string sql;
			if (wid < 0 && nickserv->IsRegistered(x[1]) == 1) {
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 320 " + nick->GetNick(sID) + " " + x[1] + " :STATUS: \0034DESCONECTADO\003.\r\n");
				if (nickserv->IsRegistered(x[1]) == 1)
					sock->Write(stream, ":" + config->Getvalue("serverName") + " 320 " + nick->GetNick(sID) + " " + x[1] + " :Tiene el nick registrado.\r\n");
				sql = "SELECT SHOWMAIL FROM OPTIONS WHERE NICKNAME='" + x[1] + "' COLLATE NOCASE;";
				if (db->SQLiteReturnInt(sql) == 1) {
					sql = "SELECT EMAIL FROM NICKS WHERE NICKNAME='" + x[1] + "' COLLATE NOCASE;";
					string email = db->SQLiteReturnString(sql);
					if (email.length() > 0)
					sock->Write(stream, ":" + config->Getvalue("serverName") + " 320 " + nick->GetNick(sID) + " " + x[1] + " :Su correo electronico es: " + email + "\r\n");
				}
				sql = "SELECT URL FROM NICKS WHERE NICKNAME='" + nick->GetNick(wid) + "' COLLATE NOCASE;";
				string url = db->SQLiteReturnString(sql);
				if (url.length() > 0)
					sock->Write(stream, ":" + config->Getvalue("serverName") + " 320 " + nick->GetNick(sID) + " " + x[1] + " :Su Web es: " + url + "\r\n");
				sql = "SELECT VHOST FROM NICKS WHERE NICKNAME='" + nick->GetNick(wid) + "' COLLATE NOCASE;";
				string vHost = db->SQLiteReturnString(sql);
				if (vHost.length() > 0)
					sock->Write(stream, ":" + config->Getvalue("serverName") + " 320 " + nick->GetNick(sID) + " " + x[1] + " :Su vHost es: " + vHost + "\r\n");
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 318 " + nick->GetNick(sID) + " " + x[1] + " :Fin de /WHOIS." + "\r\n");
				return 0;
			} else if (wid > -1) {
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 320 " + nick->GetNick(sID) + " " + nick->GetNick(wid) + " :" + nick->GetNick(wid) + " es: " + nick->GetNick(wid) + "!" + nick->GetIdent(wid) + "@" + nick->GetCloakIP(wid) + "\r\n");
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 320 " + nick->GetNick(sID) + " " + nick->GetNick(wid) + " :STATUS: \0033CONECTADO\003.\r\n");
				if (nickserv->IsRegistered(nick->GetNick(wid)) == 1)
					sock->Write(stream, ":" + config->Getvalue("serverName") + " 320 " + nick->GetNick(sID) + " " + nick->GetNick(wid) + " :Tiene el nick registrado.\r\n");
				if (oper->IsOper(nick->GetNick(sID)) == 1)
					sock->Write(stream, ":" + config->Getvalue("serverName") + " 320 " + nick->GetNick(sID) + " " + nick->GetNick(wid) + " :Su IP es: " + nick->GetIP(wid) + "\r\n");
				if (oper->IsOper(nick->GetNick(wid)) == 1)
					sock->Write(stream, ":" + config->Getvalue("serverName") + " 320 " + nick->GetNick(sID) + " " + nick->GetNick(wid) + " :Es un iRCop.\r\n");
				if (datos->nicks[wid]->tiene_z == 1)
					sock->Write(stream, ":" + config->Getvalue("serverName") + " 320 " + nick->GetNick(sID) + " " + nick->GetNick(wid) + " :Conecta mediante un canal seguro SSL.\r\n");
				if (datos->nicks[wid]->tiene_w == 1)
					sock->Write(stream, ":" + config->Getvalue("serverName") + " 320 " + nick->GetNick(sID) + " " + nick->GetNick(wid) + " :Conecta mediante WebChat.\r\n");
				if (nickserv->GetOption("SHOWMAIL", nick->GetNick(wid)) == 1) {
					sql = "SELECT EMAIL FROM NICKS WHERE NICKNAME='" + nick->GetNick(wid) + "' COLLATE NOCASE;";
					string email = db->SQLiteReturnString(sql);
					if (email.length() > 0)
					sock->Write(stream, ":" + config->Getvalue("serverName") + " 320 " + nick->GetNick(sID) + " " + nick->GetNick(wid) + " :Su correo electronico es: " + email + "\r\n");
				}
				sql = "SELECT URL FROM NICKS WHERE NICKNAME='" + nick->GetNick(wid) + "' COLLATE NOCASE;";
				string url = db->SQLiteReturnString(sql);
				if (url.length() > 0)
					sock->Write(stream, ":" + config->Getvalue("serverName") + " 320 " + nick->GetNick(sID) + " " + nick->GetNick(wid) + " :Su Web es: " + url + "\r\n");
				sql = "SELECT VHOST FROM NICKS WHERE NICKNAME='" + nick->GetNick(wid) + "' COLLATE NOCASE;";
				string vHost = db->SQLiteReturnString(sql);
				if (vHost.length() > 0)
					sock->Write(stream, ":" + config->Getvalue("serverName") + " 320 " + nick->GetNick(sID) + " " + nick->GetNick(wid) + " :Su vHost es: " + vHost + "\r\n");
				if (wid == sID && nickserv->IsRegistered(nick->GetNick(sID)) == 1) {
					string opciones;
					if (nickserv->GetOption("NOACCESS", nick->GetNick(sID)) == 1) {
						if (!opciones.empty())
							opciones.append(", ");
						opciones.append("NOACCESS");
					}
					if (nickserv->GetOption("SHOWMAIL", nick->GetNick(sID)) == 1) {
						if (!opciones.empty())
							opciones.append(", ");
						opciones.append("SHOWMAIL");
					}
					if (nickserv->GetOption("NOMEMO", nick->GetNick(sID)) == 1) {
						if (!opciones.empty())
							opciones.append(", ");
						opciones.append("NOMEMO");
					}
					if (nickserv->GetOption("NOOP", nick->GetNick(sID)) == 1) {
						if (!opciones.empty())
							opciones.append(", ");
						opciones.append("NOOP");
					}
					if (nickserv->GetOption("ONLYREG", nick->GetNick(sID)) == 1) {
						if (!opciones.empty())
							opciones.append(", ");
						opciones.append("ONLYREG");
					}
					if (opciones.length() == 0)
						opciones = "Ninguna";
					sock->Write(stream, ":" + config->Getvalue("serverName") + " 320 " + nick->GetNick(sID) + " " + nick->GetNick(wid) + " :Tus opciones son: " + opciones + "\r\n");
				}
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 318 " + nick->GetNick(sID) + " " + x[1] + " :Fin de /WHOIS." + "\r\n");
				return 0;
			} else {
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 401 " + nick->GetNick(sID) + " " + x[1] + " :El nick no existe." + "\r\n");
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 318 " + nick->GetNick(sID) + " " + x[1] + " :Fin de /WHOIS." + "\r\n");
				return 0;
			}
		}
	} else if (cmd == "WEBIRC") {
		if (x.size() < 5) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :Necesito mas datos." + "\r\n");
			return 0;
		} else if (x[1] == config->Getvalue("cgiirc")) {
			stream->cgiirc = x[4];
			return 0;
		}
	} else if (cmd == "MODE") {
		if (x.size() < 2) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :Necesito mas datos." + "\r\n");
			return 0;
		} else if (sID < 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return 0;
		} else if (nickserv->IsRegistered(nick->GetNick(sID)) == 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :Tu nick no esta registrado." + "\r\n");
			return 0;
		} else if (x[1][0] == '#') {
			if (chanserv->IsRegistered(x[1]) == 0) {
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :El canal no esta registrado." + "\r\n");
				return 0;
			} else if (chan->IsInChan(x[1], nick->GetNick(sID)) == 0) {
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :No estas dentro del canal." + "\r\n");
				return 0;
			} else if (x.size() == 2) {
				string sql = "SELECT MODOS from CANALES WHERE NOMBRE='" + x[1] + "' COLLATE NOCASE;";
				string modos = db->SQLiteReturnString(sql);
				sock->Write(stream, ":" + config->Getvalue("serverName") + " 324 " + nick->GetNick(sID) + " " + x[1] + " " + modos + "\r\n");
				return 0;
			} else if (x.size() == 3) {
				if (x[2] == "+b" || x[2] == "b") {
					int id = datos->GetChanPosition(x[1]);
					if (id < 0)
						return 0;
					for (unsigned int i = 0; i < datos->canales[id]->bans.size(); i++) {
						sock->Write(stream, ":" + config->Getvalue("serverName") + " 367 " + nick->GetNick(sID) + " " + x[1] + " " + datos->canales[id]->bans[i]->mascara + " " + datos->canales[id]->bans[i]->who + " " + to_string(datos->canales[id]->bans[i]->fecha) + "\r\n");
					}
					sock->Write(stream, ":" + config->Getvalue("serverName") + " 368 " + nick->GetNick(sID) + " " + x[1] + " :Fin de la lista de baneados." + "\r\n");
				}
			} else if (x.size() > 3) {
				if (chanserv->Access(nick->GetNick(sID), x[1]) < 2) {
					sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :No tienes acceso para banear." + "\r\n");
					return 0;
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
						
						int id = datos->GetChanPosition(x[1]);
						if (datos->canales[id]->bans.size() == 0) {
							if (action == 1) {
								datos->ChannelBan(nick->GetNick(sID), x[3+j], x[1]);
								chan->PropagarMODE(nick->FullNick(sID), x[3+j], x[1], 'b', action);
								sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El BAN ha sido fijado." + "\r\n");
								server->SendToAllServers("SMODE " + nick->GetNick(sID) + " " + x[1] + " " + mode + "b " + x[3+j]);
							} else {
								sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El canal no tiene BANs." + "\r\n");
							}
						} else {
							for (unsigned int i = 0; i < datos->canales[id]->bans.size(); i++) {
								if (datos->Match(datos->canales[id]->bans[i]->mascara.c_str(), x[3+j].c_str()) == 1) {
									match = 1;
									ban = datos->canales[id]->bans[i]->mascara;						
								}
							}
							if (match == 0) {
								if (action == 0) {
									sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El BAN no existe." + "\r\n");
								} else {
									datos->ChannelBan(nick->GetNick(sID), x[3+j], x[1]);
									chan->PropagarMODE(nick->FullNick(sID), x[3+j], x[1], 'b', action);
									server->SendToAllServers("SMODE " + nick->GetNick(sID) + " " + x[1] + " " + mode + "b " + x[3+j]);
									sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El BAN ha sido fijado." + "\r\n");
								}
							} else {
								if (action == 1) {
									sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El BAN ya existe como " + ban + "\r\n");
								} else {
									datos->UnBan(ban, x[1]);
									chan->PropagarMODE(nick->FullNick(sID), x[3+j], x[1], 'b', action);
									server->SendToAllServers("SMODE " + nick->GetNick(sID) + " " + x[1] + " " + mode + "b " + x[3+j]);
									sock->Write(stream, ":CHaN!*@* NOTICE " + nick->GetNick(sID) + " :El BAN ha sido eliminado." + "\r\n");
								}
							}
						}
						j++;
					}
				}
			}
		}
	} else if (cmd == "PING") {
		if (x.size() == 2)
			sock->Write(stream, "PONG :" + x[1] + "\r\n");
		else
			sock->Write(stream, "PONG\r\n");
		return 0;
	} else if (cmd == "NICKSERV") {
		if (sID < 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return 0;
		} else if (x.size() < 2) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :Necesito mas datos." + "\r\n");
			return 0;
		} else {
			nickserv->ProcesaMensaje(stream, mensaje.substr(9));
			return 0;
		}
	} else if (cmd == "CHANSERV") {
		if (sID < 0) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :No te has registrado." + "\r\n");
			return 0;
		} else if (x.size() < 2) {
			sock->Write(stream, ":" + config->Getvalue("serverName") + " 461 :Necesito mas datos." + "\r\n");
			return 0;
		} else {
			chanserv->ProcesaMensaje(stream, mensaje.substr(9));
			return 0;
		}
	}
	return 0;
}

void Cliente::Bienvenida (TCPStream* stream, string nickname) {
	struct tm *tm = localtime(&encendido);
	char date[30];
	strftime(date, sizeof(date), "%r %d-%m-%Y", tm);
	string fecha = date;
	sock->Write(stream, ":" + config->Getvalue("serverName") + " 001 " + nickname + " :Bienvenido a " + config->Getvalue("network") + "\r\n");
	sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 " + nickname + " :Tu Nodo es: " + config->Getvalue("serverName") + " funcionando con: " + config->version + "\r\n");
	sock->Write(stream, ":" + config->Getvalue("serverName") + " 003 " + nickname + " :Este servidor fue creado: " + fecha + "\r\n");
	sock->Write(stream, ":" + config->Getvalue("serverName") + " 004 " + nickname + " " + config->Getvalue("serverName") + " " + config->version + " rzoiws robtkmlvshn r\r\n");
	sock->Write(stream, ":" + config->Getvalue("serverName") + " 005 " + nickname + " NETWORK=" + config->Getvalue("network") + " are supported by this server\r\n");
	sock->Write(stream, ":" + config->Getvalue("serverName") + " 005 " + nickname + " NICKLEN=" + config->Getvalue("nicklen") + " MAXCHANNELS=" + config->Getvalue("maxchannels") + " CHANNELLEN=" + config->Getvalue("chanlen") + " are supported by this server\r\n");
	sock->Write(stream, ":" + config->Getvalue("serverName") + " 005 " + nickname + " PREFIX=(ohv)@%+ STATUSMSG=@%+ are supported by this server\r\n");
	sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 " + nickname + " :Hay \002" + to_string(datos->GetUsuarios()) + "\002 usuarios y \002" + to_string(datos->GetCanales()) + "\002 canales.\r\n");
	sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 " + nickname + " :Hay \002" + to_string(nickserv->GetNicks()) + "\002 nicks registrados y \002" + to_string(chanserv->GetChans()) + "\002 canales registrados.\r\n");
	sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 " + nickname + " :Hay \002" + to_string(datos->GetOperadores()) + "\002 iRCops conectados." + "\r\n");
	sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 " + nickname + " :Hay \002" + to_string(datos->GetServidores()) + "\002 servidores conectados." + "\r\n");
	return;
}
