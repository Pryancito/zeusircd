#include "include.h"
#include <cstdlib>
#include <csignal>

bool signaled = false;
pthread_mutex_t myMutex;
pthread_cond_t cond;

time_t encendido = time(0);

int main(int argc, char *argv[]) {
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
	
	datos->AddServer(NULL, config->Getvalue("serverName"), config->Getvalue("listen[0]ip"), 0);

	std::cout << "Mi Nombre es: " << config->Getvalue("serverName") << std::endl;
	
	if (ulimit(UL_SETFSIZE, MAX_USERS) < 0) {
		std::cout << "ULIMIT ERROR" << std::endl;
		exit(1);
	} else
		std::cout << "Limite de usuarios configurado a: " << MAX_USERS << std::endl;
	
	for (unsigned int i = 0; config->Getvalue("listen["+to_string(i)+"]ip").length() > 0; i++) {
		if (config->Getvalue("listen["+to_string(i)+"]class") == "client") {
			Socket *principal = new Socket();
			principal->ip = (char *) config->Getvalue("listen["+to_string(i)+"]ip").c_str();
			principal->port = (int) stoi(config->Getvalue("listen["+to_string(i)+"]port"));
			principal->SSL = (int) stoi(config->Getvalue("listen["+to_string(i)+"]ssl"));
			principal->IPv6 = 0;
			principal->tw = principal->MainThread();
			principal->tw.detach();
		} else if (config->Getvalue("listen["+to_string(i)+"]class") == "server") {
			Socket *servidores = new Socket();
			servidores->ip = (char *) config->Getvalue("listen["+to_string(i)+"]ip").c_str();
			servidores->port = (int) stoi(config->Getvalue("listen["+to_string(i)+"]port"));
			servidores->SSL = (int) stoi(config->Getvalue("listen["+to_string(i)+"]ssl"));
			servidores->IPv6 = 0;
			servidores->tw = servidores->ServerThread();
			servidores->tw.detach();
			if (server->Existe(config->Getvalue("listen["+to_string(i)+"]ip")) == 0)
				datos->AddServer(NULL, config->Getvalue("serverName"), config->Getvalue("listen["+to_string(i)+"]ip"), 0);
		}
	}
	for (unsigned int i = 0; config->Getvalue("listen6["+to_string(i)+"]ip").length() > 0; i++) {
		if (config->Getvalue("listen6["+to_string(i)+"]class") == "client") {
			Socket *principal = new Socket();
			principal->ip = (char *) config->Getvalue("listen6["+to_string(i)+"]ip").c_str();
			principal->port = (int) stoi(config->Getvalue("listen6["+to_string(i)+"]port"));
			principal->SSL = (int) stoi(config->Getvalue("listen6["+to_string(i)+"]ssl"));
			principal->IPv6 = 1;
			principal->tw = principal->MainThread();
			principal->tw.detach();
		} else if (config->Getvalue("listen6["+to_string(i)+"]class") == "server") {
			Socket *servidores = new Socket();
			servidores->ip = (char *) config->Getvalue("listen6["+to_string(i)+"]ip").c_str();
			servidores->port = (int) stoi(config->Getvalue("listen6["+to_string(i)+"]port"));
			servidores->SSL = (int) stoi(config->Getvalue("listen6["+to_string(i)+"]ssl"));
			servidores->IPv6 = 1;
			servidores->tw = servidores->ServerThread();
			servidores->tw.detach();
		}
	}
	
	std::cout << "Zeus iniciado ... OK" << std::endl;

	while (!signaled) {
   		pthread_cond_wait(&cond, &myMutex);
		procesacola ();
	}

	return 0;
}
