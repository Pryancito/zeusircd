#include "config.h"
#include "mainframe.h"

#define GC_THREADS
#define GC_ALWAYS_MULTITHREADED
#include <gc.h>
#include <gc_cpp.h>

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

void Config::Configura (string dato, const string &valor) {
	conf[dato] = valor;
	return;
}

string Config::Getvalue (string dato) {
	return conf[dato];
}

void Config::MainSocket(std::string ip, int port, bool ssl, bool ipv6) {
    GC_stack_base sb;
    GC_get_stack_base(&sb);
    GC_register_my_thread(&sb);
	try {
		Mainframe* frame = Mainframe::instance();
		frame->start(ip, port, ssl, ipv6);
	} catch (std::exception& e) {
		std::cout << "ERROR on socket" << std::endl;
	}
	GC_unregister_my_thread();
}

void Config::ServerSocket(std::string ip, int port, bool ssl, bool ipv6) {
    GC_stack_base sb;
    GC_get_stack_base(&sb);
    GC_register_my_thread(&sb);
	try {
		Mainframe* frame = Mainframe::instance();
		frame->server(ip, port, ssl, ipv6);
	} catch (std::exception& e) {
		std::cout << "ERROR on socket" << std::endl;
	}
	GC_unregister_my_thread();
}

void Config::WebSocket(std::string ip, int port, bool ssl, bool ipv6) {
    GC_stack_base sb;
    GC_get_stack_base(&sb);
    GC_register_my_thread(&sb);
	try {
		Mainframe* frame = Mainframe::instance();
		frame->ws(ip, port, ssl, ipv6);
	} catch (std::exception& e) {
		std::cout << "ERROR on socket" << std::endl;
	}
	GC_unregister_my_thread();
}
