#include "include.h"
#include <algorithm>

using namespace std;

Socket *sock = new Socket();

mutex file;

std::string invertir(const std::string &str)
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

void Socket::Write (TCPStream *stream, const string mensaje) {
	file.lock();
	if (stream != NULL && stream->getPeerSocket() > 0)
		stream->send(mensaje.c_str(), mensaje.size());
	file.unlock();
}
void Socket::MainSocket () {
	TCPStream* stream = NULL;
    TCPAcceptor* acceptor = NULL;
    acceptor = new TCPAcceptor(port, ip);

    if (acceptor->start() == 0) {
		cout << "main socket iniciado ... OK" << endl;
        while (1) {
            stream = acceptor->accept();
			if (stream == NULL)
				return;

			if (server->CheckClone(stream->getPeerIP()) == true) {
				sock->Write(stream, "Has alcanzado el limite de clones.\r\n");
				delete stream;
				return;
			}
			string hostname = stream->getPeerIP() + ".dnsbl.dronebl.org";
			hostent *record = gethostbyname(hostname.c_str());
			if(record != NULL)
			{
				oper->GlobOPs("Alerta DNSBL. DroneBL. IP: " + stream->getPeerIP() + "\r\n");
				sock->Write(stream, "Te conectas desde una conexion prohibida.\r\n");
				delete stream;
				return;
			}
			
/*			string ip = stream->getPeerIP();
			ip = invertir(ip);
			hostname = ip + ".zen.spamhaus.org";
			record = gethostbyname(hostname.c_str());
			if(record != NULL)
			{
				oper->GlobOPs("Alerta DNSBL. Spamhaus. IP: " + stream->getPeerIP() + "\r\n");
				sock->Write(stream, "Te conectas desde una conexion prohibida.\r\n");
				delete stream;
				return;
			}*/

	    	Socket *s = new Socket();
			s->tw = Thread(stream);
			s->tw.detach();
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
		} else {
			buf.append(str.substr(i, 1));
		}
	return tokens;
}

void Socket::Cliente (TCPStream* s) {
	int len;
    char line[512];
	bool quit = 0;
	do {
		bzero(line,sizeof(line));
		len = s->receive(line, sizeof(line), 300);
		line[len] = 0;
		vector <string> mensajes;
		mensajes = split_cliente(line);
		for (unsigned int i = 0; i < mensajes.size(); i++) {
			quit = cliente->ProcesaMensaje(s, mensajes[i]);
			if (quit == 1)
				break;
		}
	} while (len > 0 && quit == 0 && s != NULL);
	delete s;
	return;
}

void Socket::ServerSocket () {
	TCPStream* stream = NULL;
    TCPAcceptor* acceptor = NULL;
    acceptor = new TCPAcceptor(port, ip);

    if (acceptor->start() == 0) {
		cout << "server socket iniciado ... OK" << endl;
        while (1) {
            stream = acceptor->accept();
			if (stream == NULL)
				return;
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
	bool quit = 0;
	do {
		bzero(line,sizeof(line));
		len = s->receive(line, sizeof(line));
		line[len] = 0;
		vector <string> mensajes;
		mensajes = split_entrada(line);
		for (unsigned int i = 0; i < mensajes.size(); i++) {
			quit = server->ProcesaMensaje(s, mensajes[i]);
			if (quit == 1)
				break;
		}
	} while (len > 0 && quit == 0);
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
