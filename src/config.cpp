#include "config.h"
#include "mainframe.h"

using namespace std;

Config *config = new Config();

void Config::Cargar () {
	string linea;
	ifstream fichero(file);
	while (!fichero.eof()) {
		getline(fichero, linea);
		if (linea[0] != '#' && linea.length() > 0)
			Procesa(linea);
	}
	fichero.close();
	return;
}

void Config::Procesa (string linea) {
    vector<string> x;
    boost::split(x,linea,boost::is_any_of("=\r\n"));
	Configura(x[0], x[1]);
    return;
}

void Config::Configura (string dato, string valor) {
	conf[dato] = valor;
	return;
}

string Config::Getvalue (string dato) {
	return conf[dato];
}

void Config::MainSocket(std::string ip, int port, bool ssl, bool ipv6) {
	start:
	try {
		Mainframe* frame = Mainframe::instance();
		frame->start(ip, port, ssl, ipv6);
	} catch (std::exception& e) {
		goto start;
	}
}

void Config::ServerSocket(std::string ip, int port, bool ssl, bool ipv6) {
	start:
	try {
		Mainframe* frame = Mainframe::instance();
		frame->server(ip, port, ssl, ipv6);
	} catch (std::exception& e) {
		goto start;
	}
}

void Config::WebSocket(std::string ip, int port, bool ssl, bool ipv6) {
	start:
	try {
		Mainframe* frame = Mainframe::instance();
		frame->ws(ip, port, ssl, ipv6);
	} catch (std::exception& e) {
		goto start;
	}
}
