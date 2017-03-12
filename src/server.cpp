#include "include.h"
#include "sha256.h"

using namespace std;

Server *server = new Server();

std::vector<std::string> split(const std::string &str, int delimiter(int) = ::isspace);

int Server::GetID (string ip) {
	for (unsigned int i = 0; config->Getvalue("link["+to_string(i)+"]ip").length() > 0; i++)
		if (config->Getvalue("link["+to_string(i)+"]ip") == ip)
			return i;
	return -1;
}

int Server::GetIDS (TCPStream *stream) {
	for (unsigned int i = 0; i < datos->servers.size(); i++)
		if (datos->servers[i]->stream == stream)
			return i;
	return -1;
}

bool Server::Existe(string ip) {
	for (unsigned int i = 0; i < datos->servers.size(); i++)
		if (datos->servers[i]->ip == ip)
			return true;
	return false;
}

void Server::Conectar(string ip) {
	int id = GetID(ip);
	if (id < 0)
		return;
	string message;
	TCPConnector* connector = new TCPConnector();
	TCPStream* stream = connector->connect((char *) config->Getvalue("link["+to_string(id)+"]ip").c_str(), (int ) stoi(config->Getvalue("link["+to_string(id)+"]puerto")));
	if (stream != NULL) {
		oper->GlobOPs("Conexion con " + ip + " correcta. Sincronizando ....");
		SendBurst(stream);
		oper->GlobOPs("Fin de sincronizacion de " + ip);
		
		Socket *s = new Socket();
		s->tw = s->SThread(stream);
		s->tw.detach();
	}
	return;
}

bool Server::IsAServer (string ip) {
	int id = GetID(ip);
	if (id < 0)
		return false;
	if (config->Getvalue("link["+to_string(id)+"]ip") == ip)
			return true;
	return false;
}

bool Server::IsConected (string ip) {
	for (unsigned int i = 0; i < datos->servers.size(); i++)
		if (datos->servers[i]->ip == ip)
			return true;
	return false;
}

bool Server::HUBExiste() {
	for (unsigned int i = 0; i < datos->servers.size(); i++)
		if (datos->servers[i]->nombre == config->Getvalue("hub"))
			return true;
	return false;
}

bool Server::SoyElHUB() {
	if (config->Getvalue("serverName") == config->Getvalue("hub"))
		return true;
	else
		return false;
}
void Server::SendBurst (TCPStream* stream) {
	string hub = "HUB " + config->Getvalue("hub") + "||";
	sock->Write(stream, hub);
	
	string version = "VERSION ";
	if (db->GetLastRecord() != "") {
		version.append(db->GetLastRecord());
	} else {
		version.append("0");
	}
		
	version.append("||");
	sock->Write(stream, version);
	
	for (unsigned int i = 0; i < datos->servers.size(); i++) {
		string servidor = "SERVER " + datos->servers[i]->nombre + " " + datos->servers[i]->ip + " " + to_string(datos->servers[i]->saltos);
		for (unsigned int j = 0; j < datos->servers[i]->connected.size(); j++) {
			servidor.append(" ");
			servidor.append(datos->servers[i]->connected[j]);
		}
		servidor.append("||");
		sock->Write(stream, servidor);
	}
	for (unsigned int i = 0; i < datos->nicks.size(); i++) {
		string modos = "+";
		if (datos->nicks[i]->tiene_r)
			modos.append("r");
		if (datos->nicks[i]->tiene_z)
			modos.append("z");
		if (oper->IsOper(datos->nicks[i]->nickname) == 1)
			modos.append("o");
		sock->Write(stream, "SNICK " + datos->nicks[i]->nickname + " " + datos->nicks[i]->ip + " " + datos->nicks[i]->cloakip + " " + to_string(datos->nicks[i]->login) + " " + datos->nicks[i]->nodo + " " + modos + "||");
		sock->Write(stream, "SUSER " + datos->nicks[i]->nickname + " " + datos->nicks[i]->ident + "||");
		
		if (oper->IsOper(datos->nicks[i]->nickname) == 1)
			sock->Write(stream, "SOPER " + datos->nicks[i]->nickname + "||");
	}
	
	for (unsigned int i = 0; i < datos->canales.size(); i++)
		for (unsigned int j = 0; j < datos->canales[i]->usuarios.size(); j++)
				sock->Write(stream, "SJOIN " + datos->canales[i]->usuarios[j] + " " + datos->canales[i]->nombre + " +" + datos->canales[i]->umodes[j] + "||");
}

void Server::ListServers (TCPStream* stream) {
	for (unsigned int i = 0; i < datos->servers.size(); i++)
		sock->Write(stream, ":" + config->Getvalue("serverName") + " 002 :" + datos->servers[i]->nombre + " ( " + datos->servers[i]->ip + " )" + "\r\n");
}

bool Server::ProcesaMensaje (TCPStream* stream, const string mensaje) {
	if (mensaje.length() == 0 || mensaje == "\r\n" || mensaje == "\r" || mensaje == "\n" || mensaje == "||")
		return 0;
	vector<string> x = split(mensaje);
	string cmd = x[0];
	if (cmd == "HUB") {
		if (x.size() < 2) {
			oper->GlobOPs("No hay HUB, cerrando conexion.");
			return 1;
		} else if (config->Getvalue("hub") != x[1]) {
			oper->GlobOPs("Cerrando conexion. Los HUB no coinciden.");
			return 1;
		}
	} else if (cmd == "VERSION") {
		if (x.size() < 2) {
			oper->GlobOPs("Error en las BDDs, cerrando conexion.");
			return 1;
		} else if (db->GetLastRecord() != x[1] && server->HUBExiste() == 1) {
				oper->GlobOPs("Sincronizando BDDs.");
				int syn = db->Sync(stream, x[1]);
				oper->GlobOPs("BDDs sincronizadas, se actualizaron: " + to_string(syn) + " registros.");
				return 0;
		}
	} else if (cmd == "SERVER") {
		if (x.size() < 4) {
			oper->GlobOPs("ERROR: SERVER invalido. Cerrando conexion.");
			return 1;
		} else if (IsConected(x[1]) == 1) {
			oper->GlobOPs("ERROR: SERVER existente. Cerrando conexion.");
			return 1;
		} else {
			datos->AddServer(stream, x[1], x[2], (int ) stoi(x[3])+1);
			for (unsigned int i = 4; i < x.size(); i++)
				datos->Conexiones(x[1], x[i]);
			return 0;
		}
	} else if (cmd == "SNICK") {
		if (x.size() < 7) {
			oper->GlobOPs("SNICK Erroneo.");
			return 0;
		} else if (nick->Existe(x[1]) == 1) {
			TCPStream *streamnick = datos->BuscarStream(x[1]);
			if (streamnick != NULL)
				shutdown(streamnick->getPeerSocket(), 2);
			return 0;
		} else if (nick->Existe(x[1]) == 0) {
			datos->SNICK(x[1], x[2], x[3], stol(x[4]), x[5], x[6]);
			server->SendToAllButOne(stream, mensaje);
			return 0;
		} else {
			return 0;
		}
	} else if (cmd == "SUSER") {
		if (x.size() < 3) {
			oper->GlobOPs("SUSER Erroneo.");
			return 0;
		} else if (nick->Existe(x[1]) == 0) {
			return 0;
		} else {
			int sID = datos->BuscarIDNick(x[1]);
			if (sID < 0)
				return 0;
			nick->SetIdent(sID, x[2]);
			SendToAllButOne(stream, mensaje);
			return 0;
		}
	} else if (cmd == "SVSNICK") {
		if (x.size() < 3) {
			oper->GlobOPs("SVSNICK Erroneo.");
			return 0;
		} else if (nick->Existe(x[1]) == 0) {
			return 0;
		} else {
			chan->PropagarNick(x[1], x[2]);
			nick->CambioDeNick(datos->BuscarIDNick(x[1]), x[2]);
			SendToAllButOne(stream, mensaje);
			return 0;
		}
	} else if (cmd == "SOPER") {
		if (x.size() < 2) {
			oper->GlobOPs("SOPER Erroneo.");
			return 0;
		} else {
			datos->SetOper(x[1]);
			SendToAllButOne(stream, mensaje);
			return 0;
		}
	} else if (cmd == "QUIT") {
		if (x.size() < 2) {
			oper->GlobOPs("QUIT Erroneo.");
			return 0;
		} else {
			chan->PropagarQUITByNick(x[1]);
			datos->BorrarNickByNick(x[1]);
			SendToAllButOne(stream, mensaje);
		}
	} else if (mayus(x[1]) == "NOTICE" || mayus(x[1]) == "PRIVMSG") {
		string tmp = mensaje.substr(x[0].length()+1, mensaje.length());
		if (x[2][0] == '#') {
			chan->PropagarMSG(x[0], x[2], tmp);
		} else {
			TCPStream *nickstream = datos->BuscarStream(x[2]);
			if (nickstream == NULL)
				SendToAllButOne(stream, mensaje);
			else
				sock->Write(nickstream, ":" + nick->FullNick(datos->BuscarIDNick(x[0])) + " " + tmp);
		}
		return 0;
	} else if (cmd == "SJOIN") {
		if (x.size() < 4) {
			oper->GlobOPs("SJOIN Erroneo.");
			return 0;
		} else {
			chan->Join(x[2], x[1]);
			chan->PropagateJoin(x[2], datos->BuscarIDNick(x[1]));
			SendToAllButOne(stream, mensaje);
			if (oper->IsOper(x[1]) == 1) {
				chan->PropagarMODE(config->Getvalue("serverName"), x[1], x[2], 'o', 1);
			} else {
				int i = datos->GetChanPosition(x[2]);
				int j = datos->GetNickPosition(x[2], x[1]);
				if (datos->canales[i]->umodes[j] != 'x')
					chan->PropagarMODE(config->Getvalue("serverName"), x[1], x[2], datos->canales[i]->umodes[j], 1);
			}
		}
	} else if (cmd == "SPART") {
		if (x.size() < 3) {
			oper->GlobOPs("SPART Erroneo.");
			return 0;
		} else {
			chan->PropagatePart(x[2], x[1]);
			chan->Part(x[2], x[1]);
			SendToAllButOne(stream, mensaje);
			return 0;
		}
	} else if (cmd == "DB") {
		string sql = mensaje.substr(20);
		db->SQLiteNoReturn(sql);
		db->AlmacenaDB(mensaje);
		server->SendToAllButOne(stream, mensaje);
		return 0;
	} else if (cmd == "SQUIT") {
		if (x.size() == 2) {
			for (unsigned int i = 0; i < datos->servers.size(); i++)
				if (datos->servers[i]->nombre == x[1]) {
					for (unsigned int j = 0; j < datos->servers[i]->connected.size(); j++) {
						server->SQUITByServer(datos->servers[i]->connected[j]);
						datos->DeleteServer(datos->servers[i]->connected[j]);
					}
					server->SQUITByServer(x[1]);
					datos->DeleteServer(x[1]);
				}
			server->SendToAllButOne(stream, mensaje);
			return 0;
		}
	}
	return 0;
}

void Server::SQUITByServer(string server) {
	for (unsigned int i = 0; i < datos->nicks.size(); i++)
		if (datos->nicks[i]->nodo == server) {
			chan->PropagarQUITByNick(datos->nicks[i]->nickname);
			datos->BorrarNickByNick(datos->nicks[i]->nickname);
		}
	return;
}

void Server::SendToAllServers (const string std) {
	for (unsigned int i = 0; i < datos->servers.size(); i++)
		if (datos->servers[i]->stream != NULL)
			sock->Write(datos->servers[i]->stream, std + "||");
}

void Server::SendToAllButOne (TCPStream *stream, const string std) {
	for (unsigned int i = 0; i < datos->servers.size(); i++)
		if (datos->servers[i]->stream != NULL && datos->servers[i]->stream != stream)
			sock->Write(datos->servers[i]->stream, std + "||");
}

bool Server::CheckClone(string ip) {
	int count = 1;
	for (unsigned int i = 0; i < datos->nicks.size(); i++)
		if (datos->nicks[i]->ip == ip)
			count++;
	if (count > stoi(config->Getvalue("clones")))
		return true;
	else
		return false;
}

string Server::FindName(string ip) {
	for (unsigned int i = 0; i < datos->servers.size(); i++)
		if (datos->servers[i]->ip == ip)
			return datos->servers[i]->nombre;
	return "";
}

bool Server::IsAServerTCP(TCPStream *stream) {
	for (unsigned int i = 0; i < datos->servers.size(); i++)
		if (datos->servers[i]->stream == stream)
			return true;
	return false;
}

string Server::GetServerTCP (TCPStream *stream) {
	for (unsigned int i = 0; i < datos->servers.size(); i++)
		if (datos->servers[i]->stream == stream)
			return datos->servers[i]->nombre;
	return "";
}
