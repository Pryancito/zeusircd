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
    system("rm -f zeus.pid");
    return;
}

void timeouts () {
	time_t now = time(0);
	for (User *u = users.first(); u != NULL; u = users.next(u)) {
		if (u->GetLastPing() + 90 < now)
			u->GetSocket(u->GetNick())->Write("PING :" + config->Getvalue("serverName") + "\r\n");
		if (u->GetLastPing() + 270 < now) {
			Socket *sck = user->GetSocket(u->GetNick());
			vector <UserChan*> temp;
			for (UserChan *uc = usuarios.first(); uc != NULL; uc = usuarios.next(uc)) {
				if (boost::iequals(uc->GetID(), u->GetID(), loc)) {
					chan->PropagarQUIT(u, uc->GetNombre());
					temp.push_back(uc);
				}
			}
			for (unsigned int i = 0; i < temp.size(); i++) {
				UserChan *uc = temp[i];
				usuarios.del(uc);
				if (chan->GetUsers(uc->GetNombre()) == 0) {
					chan->DelChan(uc->GetNombre());
				}
			}

			for (User *usr = users.first(); usr != NULL; usr = users.next(usr)) {
				if (boost::iequals(usr->GetID(), u->GetID(), loc)) {
					users.del(usr);
					break;
				}
			}
			for (Socket *socket = sock.first(); socket != NULL; socket = sock.next(socket)) {
				if (boost::iequals(socket->GetID(), sck->GetID(), loc)) {
					sck->Close();
					sock.del(socket);
					break;
				}
			}
		}
	}
	int expire = (int ) stoi(config->Getvalue("banexpire"));
	for (BanChan *bc = bans.first(); bc != NULL; bc = bans.next(bc))
		if (bc->GetTime() + (expire * 60) < now) {
			chan->UnBan(bc->GetMask(), bc->GetNombre());
			chan->PropagarMODE(config->Getvalue("serverName"), bc->GetMask(), bc->GetNombre(), 'b', 0);
		}
}

void write_pid () {
	ofstream procid("zeus.pid");
	procid << getpid() << endl;
	procid.close();
}

int main(int argc, char *argv[]) {
	bool demonio = true;
	if (argc == 1) {
		std::cout << "Has iniciado mal el ircd. Para ayuda consulta: ./Zeus -h" << std::endl;
		exit(0);
	}
	for (int i = 1; i < argc; i++) {
		if (boost::iequals(argv[i], "-h") && argc == 2) {
			std::cout << "Uso: " << argv[0] << " [-c server.conf] [-p password] -f [-start|-stop|-restart]" << std::endl;
			std::cout << "Iniciar: ./Zeus -start | Parar: ./Zeus -stop | Reiniciar: ./Zeus -restart" << std::endl;
			exit(0);
		} if (boost::iequals(argv[i], "-c") && argc > 2) {
			if (access(argv[i+1], W_OK) != 0) {
				std::cout << "Error al cargar el archivo de configuraciones." << std::endl;
				exit(0);
			} else {
				config->file = argv[i+1];
				continue;
			}
		} if (boost::iequals(argv[i], "-p") && argc > 2) {
			std::cout << "Password "<< argv[i+1] << " encriptada: " << sha256(argv[i+1]) << std::endl;
			exit(0);
		} if (boost::iequals(argv[i], "-start")) {
			if (access("zeus.pid", W_OK) != 0)
				continue;
			else {
				std::cout << "El servidor ya esta iniciado, si no es asi borre el archivo 'zeus.pid'" << std::endl;
				exit(0);
			}
		} else if (boost::iequals(argv[i], "-stop")) {
			if (access("zeus.pid", W_OK) == 0) {
				system("kill -9 `cat zeus.pid`");
				system("rm -f zeus.pid");
				exit(0);
			} else {
				std::cout << "El servidor no esta iniciado, si no es asi parelo manualmente." << std::endl;
				exit(0);
			}
		} else if (boost::iequals(argv[i], "-restart")) {
			if (access("zeus.pid", W_OK) == 0)
				system("kill -9 `cat zeus.pid`");
			continue;
		} else if (boost::iequals(argv[i], "-f")) {
			demonio = false;
			continue;
		}
	}

	config->Cargar();

	std::cout << "Mi Nombre es: " << config->Getvalue("serverName") << std::endl;
	std::cout << "Zeus iniciado ... OK" << std::endl;

	if (demonio == true)
		daemon(1, 0);
	
	write_pid();

	std::atexit(exiting);
	
	std::locale loc(config->Getvalue("locale").c_str());

	if (access("zeus.db", W_OK) != 0)
		db->IniciarDB();
	
	srand(time(0));
	
	if (ulimit(UL_SETFSIZE, MAX_USERS) < 0) {
		std::cout << "ULIMIT ERROR" << std::endl;
		exit(1);
	} else
		std::cout << "Limite de usuarios configurado a: " << MAX_USERS << std::endl;
	
	for (unsigned int i = 0; config->Getvalue("listen["+boost::to_string(i)+"]ip").length() > 0; i++) {
		if (config->Getvalue("listen["+boost::to_string(i)+"]class") == "client") {
			boost::asio::io_service io_service;
			boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
			Socket *principal = new Socket(io_service, ctx);
			principal->SetIP(config->Getvalue("listen["+boost::to_string(i)+"]ip"));
			principal->SetPort((int) stoi(config->Getvalue("listen["+boost::to_string(i)+"]port")));
			if (config->Getvalue("listen["+boost::to_string(i)+"]ssl") == "1" || config->Getvalue("listen["+boost::to_string(i)+"]ssl") == "true")
				principal->SetSSL(1);
			else
				principal->SetSSL(0);
			principal->SetIPv6(0);
			principal->SetID();
			principal->tw = new boost::thread(boost::bind(&Socket::MainSocket, principal));
			principal->tw->detach();
		} else if (config->Getvalue("listen["+boost::to_string(i)+"]class") == "server") {
			boost::asio::io_service io_service;
			boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
			Socket *srv = new Socket(io_service, ctx);
			srv->SetIP(config->Getvalue("listen["+boost::to_string(i)+"]ip"));
			srv->SetPort((int) stoi(config->Getvalue("listen["+boost::to_string(i)+"]port")));
			if (config->Getvalue("listen["+boost::to_string(i)+"]ssl") == "1" || config->Getvalue("listen["+boost::to_string(i)+"]ssl") == "true")
				srv->SetSSL(1);
			else
				srv->SetSSL(0);
			srv->SetIPv6(0);
			srv->SetID();
			srv->tw = new boost::thread(boost::bind(&Socket::ServerSocket, srv));
			srv->tw->detach();
			Servidor *xs = new Servidor(NULL, config->Getvalue("serverID"));
				xs->SetNombre(config->Getvalue("serverName"));
				xs->SetIP(config->Getvalue("listen["+boost::to_string(i)+"]ip"));
				xs->SetSaltos(0);
			servidores.add(xs);
		}
	}
	for (unsigned int i = 0; config->Getvalue("listen6["+boost::to_string(i)+"]ip").length() > 0; i++) {
		if (config->Getvalue("listen6["+boost::to_string(i)+"]class") == "client") {
			boost::asio::io_service io_service;
			boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
			Socket *principal = new Socket(io_service, ctx);
			principal->SetIP(config->Getvalue("listen6["+boost::to_string(i)+"]ip"));
			principal->SetPort((int) stoi(config->Getvalue("listen6["+boost::to_string(i)+"]port")));
			if (config->Getvalue("listen6["+boost::to_string(i)+"]ssl") == "1" || config->Getvalue("listen6["+boost::to_string(i)+"]ssl") == "true")
				principal->SetSSL(1);
			else
				principal->SetSSL(0);
			principal->SetIPv6(1);
			principal->SetID();
			principal->tw = new boost::thread(boost::bind(&Socket::MainSocket, principal));
			principal->tw->detach();
		} else if (config->Getvalue("listen6["+boost::to_string(i)+"]class") == "server") {
			boost::asio::io_service io_service;
			boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
			Socket *srv = new Socket(io_service, ctx);
			srv->SetIP(config->Getvalue("listen6["+boost::to_string(i)+"]ip"));
			srv->SetPort((int) stoi(config->Getvalue("listen6["+boost::to_string(i)+"]port")));
			if (config->Getvalue("listen6["+boost::to_string(i)+"]ssl") == "1" || config->Getvalue("listen6["+boost::to_string(i)+"]ssl") == "true")
				srv->SetSSL(1);
			else
				srv->SetSSL(0);
			srv->SetIPv6(1);
			srv->SetID();
			srv->tw = new boost::thread(boost::bind(&Socket::ServerSocket, srv));
			srv->tw->detach();
			Servidor *xs = new Servidor(NULL, config->Getvalue("serverID"));
				xs->SetNombre(config->Getvalue("serverName"));
				xs->SetIP(config->Getvalue("listen6["+boost::to_string(i)+"]ip"));
				xs->SetSaltos(0);
			servidores.add(xs);
		}
	}
		
	while (1) {
		sleep(20);
		timeouts();
	}
	return 0;
}
