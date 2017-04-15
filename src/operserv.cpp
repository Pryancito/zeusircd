#include "include.h"

using namespace std;

OperServ *operserv = new OperServ();

std::vector<std::string> split(const std::string &str, int delimiter(int) = ::isspace);

void OperServ::ProcesaMensaje(TCPStream *stream, string mensaje) {
	if (mensaje.length() == 0 || mensaje == "\r\n" || mensaje == "\r" || mensaje == "\n" || mensaje == "||")
		return;
	vector<string> x = split(mensaje);
	string cmd = x[0];
	mayuscula(cmd);
	int sID = datos->BuscarIDStream(stream);
	
	if (cmd == "GLINE") {
		
	}
}
