#include "include.h"
#include <cstdlib>
#include <csignal>

bool signaled = false;
pthread_mutex_t myMutex;
pthread_cond_t cond;

int main(int argc, char *argv[]) {
	Socket principal;
	Socket servidores;

	auto lam = [] (int i) {
		server->SendToAllServers("SQUIT " + config->Getvalue("serverName"));
		exit(0);
	};
	
	signal(SIGINT, lam);
    signal(SIGABRT, lam);
    signal(SIGTERM, lam);
    signal(SIGTSTP, lam);
	
	config->Cargar();

	if (access("zeus.db", W_OK) != 0)
		db->IniciarDB();
	
	srand(time(0));
	
	datos->AddServer(NULL, config->Getvalue("serverName"), config->Getvalue("listenServer"), 0);

	std::cout << "Mi Nombre es: " << config->Getvalue("serverName") << std::endl;
	
	if (ulimit(UL_SETFSIZE, MAX_USERS) < 0) {
		std::cout << "ULIMIT ERROR" << std::endl;
		exit(1);
	} else
		std::cout << "Limite de usuarios configurado a: " << MAX_USERS << std::endl;
	
	principal.ip = (char *) config->Getvalue("listen").c_str();
	principal.port = (int) stoi(config->Getvalue("puerto"));

	principal.tw = principal.MainThread();
	principal.tw.detach();
	
	servidores.ip = (char *) config->Getvalue("listenServer").c_str();
	servidores.port = (int) stoi(config->Getvalue("puertoServer"));

	servidores.tw = servidores.ServerThread();
	servidores.tw.detach();
	
	std::cout << "Zeus iniciado ... OK" << std::endl;

	while (!signaled) {
   		pthread_cond_wait(&cond, &myMutex);
		procesacola ();
	}

	return 0;
}
