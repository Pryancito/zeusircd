#include "include.h"
#include <cstdlib>
#include "sha256.h"
#include "../src/lista.cpp"
#include "../src/nodes.cpp"

time_t encendido = time(0);
std::locale loc;

using namespace std;

void exiting () {
	server->SendToAllServers("SQUIT " + config->Getvalue("serverID") + "||");
	for (Socket *s = sock.first(); s != NULL; s = sock.next(s)) {
		s->tw->join();
		delete s->tw;
		delete s;
	}
	usuarios.del_all();
	canales.del_all();
	users.del_all();
	servidores.del_all();
	sock.del_all();
    delete config;
    delete user;
    delete chan;
    delete oper;
    delete db;
}

int main(int argc, char *argv[]) {
	
	std::atexit(exiting);
	
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-h") == 0) {
			std::cout << "Uso: " << argv[0] << " [-f server.conf] [-p password]" << std::endl;
			return 0;
		} else if (strcmp(argv[i], "-f") == 0) {
			if (access(argv[i+1], W_OK) != 0) {
				std::cout << "Error al cargar el archivo de configuraciones." << std::endl;
				return 0;
			} else
				config->file = argv[i+1];
		} else if (strcmp(argv[i], "-p") == 0) {
			std::cout << "Password "<< argv[i+1] << " encriptada: " << sha256(argv[i+1]) << std::endl;
			return 0;
		}	
	}
	config->Cargar();

	std::locale loc(config->Getvalue("locale").c_str());

	if (access("zeus.db", W_OK) != 0)
		db->IniciarDB();
	
	srand(time(0));

	std::cout << "Mi Nombre es: " << config->Getvalue("serverName") << std::endl;
	
	if (ulimit(UL_SETFSIZE, MAX_USERS) < 0) {
		std::cout << "ULIMIT ERROR" << std::endl;
		exit(1);
	} else
		std::cout << "Limite de usuarios configurado a: " << MAX_USERS << std::endl;
	
	for (unsigned int i = 0; config->Getvalue("listen["+to_string(i)+"]ip").length() > 0; i++) {
		if (config->Getvalue("listen["+to_string(i)+"]class") == "client") {
			boost::asio::io_service io_service;
			boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
			Socket *principal = new Socket(io_service, ctx);
			principal->SetIP(config->Getvalue("listen["+to_string(i)+"]ip"));
			principal->SetPort((int) stoi(config->Getvalue("listen["+to_string(i)+"]port")));
			if (config->Getvalue("listen["+to_string(i)+"]ssl") == "1" || config->Getvalue("listen["+to_string(i)+"]ssl") == "true")
				principal->SetSSL(1);
			else
				principal->SetSSL(0);
			principal->SetIPv6(0);
			principal->tw = new boost::thread(boost::bind(&Socket::MainSocket, principal));
			principal->tw->detach();
		} else if (config->Getvalue("listen["+to_string(i)+"]class") == "server") {
			boost::asio::io_service io_service;
			boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
			Socket *srv = new Socket(io_service, ctx);
			srv->SetIP(config->Getvalue("listen["+to_string(i)+"]ip"));
			srv->SetPort((int) stoi(config->Getvalue("listen["+to_string(i)+"]port")));
			if (config->Getvalue("listen["+to_string(i)+"]ssl") == "1" || config->Getvalue("listen["+to_string(i)+"]ssl") == "true")
				srv->SetSSL(1);
			else
				srv->SetSSL(0);
			srv->SetIPv6(0);
			srv->tw = new boost::thread(boost::bind(&Socket::ServerSocket, srv));
			srv->tw->detach();
			Servidor *xs = new Servidor(NULL, config->Getvalue("serverID"));
				xs->SetNombre(config->Getvalue("serverName"));
				xs->SetIP(config->Getvalue("listen["+to_string(i)+"]ip"));
				xs->SetSaltos(0);
			servidores.add(xs);
		}
	}
	for (unsigned int i = 0; config->Getvalue("listen6["+to_string(i)+"]ip").length() > 0; i++) {
		if (config->Getvalue("listen6["+to_string(i)+"]class") == "client") {
			boost::asio::io_service io_service;
			boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
			Socket *principal = new Socket(io_service, ctx);
			principal->SetIP(config->Getvalue("listen6["+to_string(i)+"]ip"));
			principal->SetPort((int) stoi(config->Getvalue("listen6["+to_string(i)+"]port")));
			if (config->Getvalue("listen6["+to_string(i)+"]ssl") == "1" || config->Getvalue("listen6["+to_string(i)+"]ssl") == "true")
				principal->SetSSL(1);
			else
				principal->SetSSL(0);
			principal->SetIPv6(1);
			principal->tw = new boost::thread(boost::bind(&Socket::MainSocket, principal));
			principal->tw->detach();
		} else if (config->Getvalue("listen6["+to_string(i)+"]class") == "server") {
			boost::asio::io_service io_service;
			boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
			Socket *srv = new Socket(io_service, ctx);
			srv->SetIP(config->Getvalue("listen6["+to_string(i)+"]ip"));
			srv->SetPort((int) stoi(config->Getvalue("listen6["+to_string(i)+"]port")));
			if (config->Getvalue("listen6["+to_string(i)+"]ssl") == "1" || config->Getvalue("listen6["+to_string(i)+"]ssl") == "true")
				srv->SetSSL(1);
			else
				srv->SetSSL(0);
			srv->SetIPv6(1);
			srv->tw = new boost::thread(boost::bind(&Socket::ServerSocket, srv));
			srv->tw->detach();
			Servidor *xs = new Servidor(NULL, config->Getvalue("serverID"));
				xs->SetNombre(config->Getvalue("serverName"));
				xs->SetIP(config->Getvalue("listen6["+to_string(i)+"]ip"));
				xs->SetSaltos(0);
			servidores.add(xs);
		}
	}
	std::cout << "Zeus iniciado ... OK" << std::endl;
	
	while (1) {
		sleep(200);
//  		semaforo.wait();
//		procesacola ();
	}
	return 0;
}
