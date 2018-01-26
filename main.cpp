#include "include.h"
#include <cstdlib>
#include "sha256.h"
#include "api.h"
#include "../src/lista.cpp"
#include "../src/nodes.cpp"

time_t encendido = time(0);
std::locale loc;
boost::thread *th_api;

using namespace std;
using namespace ourapi;

void exiting (int signo) {
	Servidor::SendToAllServers("SQUIT " + config->Getvalue("serverID") + " " + config->Getvalue("serverID"));
	for (Socket *s = sock.first(); s != NULL; s = sock.next(s)) {
		s->tw->join();
		delete s->tw;
		delete s;
	}
	shouldNotExit = 0;
	usuarios.del_all();
	canales.del_all();
	users.del_all();
	servidores.del_all();
	sock.del_all();
	delete config;
	delete th_api;
    system("rm -f zeus.pid");
    return;
}

void timeouts () {
	time_t now = time(0);
	for (User *u = users.first(); u != NULL; u = users.next(u)) {
		if (User::GetSocketByID(u->GetID()) == NULL)
			continue;
		if (u->GetLastPing() + 90 < now)
			u->GetSocket(u->GetNick())->Write("PING :" + config->Getvalue("serverName") + "\r\n");
		if (u->GetLastPing() + 270 < now) {
			Socket *sck = User::GetSocket(u->GetNick());
			User::Quit(u, sck);
		}
	}
	int expire = (int ) stoi(config->Getvalue("banexpire"));
	for (BanChan *bc = bans.first(); bc != NULL; bc = bans.next(bc))
		if (bc->GetTime() + (expire * 60) < now) {
			Chan::UnBan(bc->GetMask(), bc->GetNombre());
			Chan::PropagarMODE(config->Getvalue("serverName"), bc->GetMask(), bc->GetNombre(), 'b', 0, 1);
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

	if (ulimit(UL_SETFSIZE, MAX_USERS) < 0) {
		std::cout << "ULIMIT ERROR" << std::endl;
		exit(1);
	} else
		std::cout << "Limite de usuarios configurado a: " << ulimit(UL_GETFSIZE) << std::endl;

	if (demonio == true)
		daemon(1, 0);
	
	write_pid();

	signal(SIGTERM, exiting);
	signal(SIGKILL, exiting);
	
	std::locale loc(config->Getvalue("locale").c_str());

	if (access("zeus.db", W_OK) != 0)
		DB::IniciarDB();
	
	OperServ::ApplyGlines();
	
	srand(time(0));
	
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
	if (config->Getvalue("hub") == config->Getvalue("serverName") && (config->Getvalue("api") == "true" || config->Getvalue("api") == "1"))
		th_api = new boost::thread(api::http);
	
	while (1) {
		sleep(20);
		timeouts();
	}
	return 0;
}
