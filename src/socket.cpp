#include "include.h"
#include <algorithm>
#include <queue>
#include <semaphore.h>

using namespace std;

Socket *sock = new Socket();

mutex file;
std::map <TCPStream *, int> flood;
long int lastflood;
std::queue <infocola> cola;

void procesacola () {
	if (lastflood + 20 < static_cast<long int> (time(NULL))) {
		for (unsigned int i = 0; i < datos->nicks.size(); i++) {
			flood[datos->nicks[i]->stream] = 0;
		}
		lastflood = static_cast<long int> (time(NULL));
	}
	while (!cola.empty()) {
		bool quit = 0;
		infocola data;
		data = cola.front();
		if (server->IsAServerTCP(data.stream) == 1)
			quit = server->ProcesaMensaje(data.stream, data.mensaje);
		else if (flood[data.stream] != -1) {
			flood[data.stream] += data.mensaje.length();
			if (flood[data.stream] > SENDQ) {
				shutdown(data.stream->getPeerSocket(), 2);
				flood[data.stream] = -1;
				continue;
			}
			quit = cliente->ProcesaMensaje(data.stream, data.mensaje);
		} else
			quit = 1;
		if (quit == 1) {
			shutdown(data.stream->getPeerSocket(), 2);
		}
		cola.pop();
	}

	for (unsigned int i = 0; i < datos->canales.size(); i++)
		for (unsigned int j = 0; j < datos->canales[i]->bans.size(); j++) {
			unsigned long now = static_cast<long int> (time(NULL));
			unsigned long expire = static_cast<long int> (stoi(config->Getvalue("banexpire")));
			if (datos->canales[i]->bans[j]->fecha + (expire * 60) < now) {
				datos->UnBan(datos->canales[i]->bans[j]->mascara, datos->canales[i]->nombre);
				chan->PropagarMODE(config->Getvalue("serverName"), datos->canales[i]->bans[j]->mascara, datos->canales[i]->nombre, 'b', 0);
			}
		}
	semaforo.close();
}

std::string invertir(const std::string str)
{
    std::string rstr = str;
    std::reverse(rstr.begin(), rstr.end());
    int size = rstr.size();
    int start = 0;
    int end = 0;
    while (end != size + 1) {
        if (rstr[end] == '.' || end == size) {
            std::reverse(rstr.begin() + start, rstr.begin() + end);
            start = end + 1;
        }
        ++end;
    }
    return rstr;
}

std::string BinToHex(const void* raw, size_t l)
{
	static const char hextable[] = "0123456789abcdef";
	const char* data = static_cast<const char*>(raw);
	std::string rv;
	rv.reserve(l * 2);
	for (size_t i = 0; i < l; i++)
	{
		unsigned char c = data[i];
		rv.push_back(hextable[c >> 4]);
		rv.push_back(hextable[c & 0xF]);
	}
	return rv;
}

std::string invertirv6 (const std::string str) {
	struct in6_addr addr;
    inet_pton(AF_INET6,str.c_str(),&addr);
	const unsigned char* ip = addr.s6_addr;
	std::string reversedip;

	std::string buf = BinToHex(ip, 16);
	for (std::string::const_reverse_iterator it = buf.rbegin(); it != buf.rend(); ++it)
	{
		reversedip.push_back(*it);
		reversedip.push_back('.');
	}
	return reversedip;
}

void Socket::Write (TCPStream *stream, const string mensaje) {
	file.lock();
	if (stream != NULL && stream->getPeerSocket() > 0 && flood[stream] != -1) {
		stream->send(mensaje.c_str(), mensaje.size());
	}
	file.unlock();
}

void Socket::MainSocket () {
	TCPAcceptor6* acceptor6 = NULL;
    acceptor6 = new TCPAcceptor6(port, ip);
	TCPAcceptor* acceptor = NULL;
    acceptor = new TCPAcceptor(port, ip);
    string ipcliente;
	if (IPv6 == 1) {
	    if (acceptor6->start() == 0) {
			cout << "client socket iniciado " << ip << "@" << port << " ... OK" << endl;
	        while (1) {
	        	bool baned = 0;
	        	TCPStream* stream = NULL;
	            stream = acceptor6->accept(SSL);
				if (stream == NULL)
					continue;
	
				if (server->CheckClone(stream->getPeerIP()) == true) {
					sock->Write(stream, "Has alcanzado el limite de clones.\r\n");
					delete stream;
					continue;
				}
				
				for (unsigned int i = 0; config->Getvalue("dnsbl6["+to_string(i)+"]suffix").length() > 0; i++) {
					if (config->Getvalue("dnsbl6["+to_string(i)+"]reverse") == "true") {
						ipcliente = invertirv6(stream->getPeerIP());
					} else {
						ipcliente = stream->getPeerIP();
					}
					string hostname = ipcliente + config->Getvalue("dnsbl6["+to_string(i)+"]suffix");
					hostent *record = gethostbyname(hostname.c_str());
					if(record != NULL)
					{
						oper->GlobOPs("Alerta DNSBL. " + config->Getvalue("dnsbl6["+to_string(i)+"]suffix") + " IP: " + stream->getPeerIP() + "\r\n");
						sock->Write(stream, ":" + config->Getvalue("serverName") + " :Te conectas desde una conexion prohibida.\r\n");
						baned = 1;
						delete stream;
						continue;
					}
				}
				if (baned == 1)
					continue;
		    	Socket *s = new Socket();
				s->tw = Thread(stream);
				s->tw.detach();
	        }
	    }
	} else {
		if (acceptor->start() == 0) {
			cout << "client socket iniciado " << ip << "@" << port << " ... OK" << endl;
	        while (1) {
	        	bool baned = 0;
	        	TCPStream* stream = NULL;
	            stream = acceptor->accept(SSL);
				if (stream == NULL)
					continue;
	
				if (server->CheckClone(stream->getPeerIP()) == true) {
					sock->Write(stream, "Has alcanzado el limite de clones.\r\n");
					delete stream;
					continue;
				}
				for (unsigned int i = 0; config->Getvalue("dnsbl["+to_string(i)+"]suffix").length() > 0; i++) {
					if (config->Getvalue("dnsbl["+to_string(i)+"]reverse") == "true") {
						ipcliente = invertir(stream->getPeerIP());
					} else {
						ipcliente = stream->getPeerIP();
					}
					string hostname = ipcliente + config->Getvalue("dnsbl["+to_string(i)+"]suffix");
					hostent *record = gethostbyname(hostname.c_str());
					if(record != NULL)
					{
						oper->GlobOPs("Alerta DNSBL. " + config->Getvalue("dnsbl["+to_string(i)+"]suffix") + " IP: " + stream->getPeerIP() + "\r\n");
						sock->Write(stream, ":" + config->Getvalue("serverName") + " :Te conectas desde una conexion prohibida.\r\n");
						baned = 1;
						delete stream;
						break;
					}
				}
				if (baned == 1)
					continue;
		    	Socket *s = new Socket();
				s->tw = Thread(stream);
				s->tw.detach();
	        }
	    }
	}
}

std::vector<std::string> split_cliente(const std::string &str){
	vector <string> tokens;
	string buf;
	for (unsigned int i = 0; i < str.length(); i++)
		if (str[i] == '\r' && str[i+1] == '\n') {
			if (!buf.empty()) {
				tokens.push_back(buf);
				buf.clear();
				i++;
			}
		} else if (str[i] == '\r' && str[i+1] != '\n') {
			if (!buf.empty()) {
				tokens.push_back(buf);
				buf.clear();
			}
		} else if (str[i] == '\n') {
			if (!buf.empty()) {
				tokens.push_back(buf);
				buf.clear();
			}
		} else {
			buf.append(str.substr(i, 1));
		}
	return tokens;
}

void Socket::Cliente (TCPStream* s) {
	int len;
    char line[512];
	do {
		bzero(line,sizeof(line));
		len = s->receive(line, sizeof(line));
		semaforo.open();
		line[len] = 0;
		vector <string> mensajes;
		mensajes = split_cliente(line);
		for (unsigned int i = 0; i < mensajes.size(); i++) {
			infocola datos;
			datos.stream = s;
			datos.mensaje = mensajes[i];
			cola.push(datos);
			semaforo.notify();
		}
	} while (len > 0 && s->getPeerSocket() > 0 && flood[s] < SENDQ);
	delete s;
	return;
}

void Socket::ServerSocket () {
	TCPStream* stream = NULL;
	TCPAcceptor6* acceptor6 = NULL;
    acceptor6 = new TCPAcceptor6(port, ip);
	TCPAcceptor* acceptor = NULL;
   	acceptor = new TCPAcceptor(port, ip);

	if (IPv6 == 1) {
	    if (acceptor6->start() == 0) {
			cout << "server socket iniciado " << ip << "@" << port << " ... OK" << endl;
	        while (1) {
	            stream = acceptor6->accept(SSL);
				if (stream != NULL) {
					string ipe = stream->getPeerIP();
					if (server->IsAServer(ipe) == 0) {
						oper->GlobOPs("Intento de conexion de :" +ipe + " - No se encontro en la configuracion.");
						delete stream;
					} else if (server->IsConected(ipe) == 1) {
						delete stream;
						oper->GlobOPs("El servidor " + ipe + " ya existe, se ha ignorado el intento de conexion.");
					} else {
						server->SendBurst(stream);
						oper->GlobOPs("Fin de sincronizacion de " + ipe);
						
						Socket *s = new Socket();
						s->tw = SThread(stream);
						s->tw.detach();
		            }
				} else
					return;
	        }
	    }
	} else {
		if (acceptor->start() == 0) {
			cout << "server socket iniciado " << ip << "@" << port << " ... OK" << endl;
	        while (1) {
	            stream = acceptor->accept(SSL);
				if (stream != NULL) {
					string ipe = stream->getPeerIP();
					if (server->IsAServer(ipe) == 0) {
						oper->GlobOPs("Intento de conexion de :" +ipe + " - No se encontro en la configuracion.");
						delete stream;
					} else if (server->IsConected(ipe) == 1) {
						delete stream;
						oper->GlobOPs("El servidor " + ipe + " ya existe, se ha ignorado el intento de conexion.");
					} else {
						server->SendBurst(stream);
						oper->GlobOPs("Fin de sincronizacion de " + ipe);
						
						Socket *s = new Socket();
						s->tw = SThread(stream);
						s->tw.detach();
		            }
				} else
					return;
	        }
	    }
	}
}

std::vector<std::string> split_entrada(const std::string &str){
	vector <string> tokens;
	string buf;
	for (unsigned int i = 0; i < str.length(); i++)
		if (str[i] == '|' && str[i+1] == '|') {
			if (!buf.empty()) {
				tokens.push_back(buf);
				buf.clear();
				i++;
			}
		} else {
			buf.append(str.substr(i, 1));
		}
	return tokens;
}

void Socket::Servidor (TCPStream* s) {
	int len;
    char line[512];
	do {
		bzero(line,sizeof(line));
		len = s->receive(line, sizeof(line));
		semaforo.open();
		line[len] = 0;
		vector <string> mensajes;
		mensajes = split_entrada(line);
		for (unsigned int i = 0; i < mensajes.size(); i++) {
			infocola datos;
			datos.stream = s;
			datos.mensaje = mensajes[i];
			cola.push(datos);
			semaforo.notify();
		}
	} while (len > 0 && s->getPeerSocket() > 0);
	delete s;
	return;
}

thread Socket::MainThread() {
    return thread([=] { MainSocket(); });
}

thread Socket::Thread(TCPStream* s) {
    return thread([=] { Cliente(s); });
}

thread Socket::ServerThread() {
    return thread([=] { ServerSocket(); });
}

thread Socket::SThread(TCPStream* s) {
    return thread([=] { Servidor(s); });
}
