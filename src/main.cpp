#include <iostream>
#include <boost/thread.hpp>
#include <ulimit.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <ulimit.h>

#include "config.h"
#include "server.h"
#include "mainframe.h"
#include "sha256.h"
#include "db.h"
#include "services.h"
#include "utils.h"

#define MAX_USERS 65000

#include "api.h"

time_t encendido = time(0);
boost::thread *th_api;

using namespace std;
using namespace ourapi;

extern ServerSet Servers;
extern CloneMap mThrottle;

void write_pid () {
	ofstream procid("zeus.pid");
	procid << getpid() << endl;
	procid.close();
}
void exit() {
	delete th_api;
	delete config;
	system("rm -f zeus.pid");
}
void timeouts () {
	time_t now = time(0);
	UserMap user = Mainframe::instance()->users();
	UserMap::iterator it = user.begin();
	for (; it != user.end(); ++it) {
		if (!it->second)
			continue;
		else if (it->second->GetPing() != 0 && it->second->GetPing() + 3600 < now && it->second->session() != nullptr && it->second->getMode('w') == true)
			it->second->cmdQuit();
		else if (it->second->GetPing() != 0 && it->second->GetPing() + 180 < now && it->second->session() != nullptr)
			it->second->cmdQuit();
		else if (it->second->GetPing() + 60 < now && it->second->session() != nullptr)
			it->second->session()->send("PING :" + config->Getvalue("serverName") + config->EOFMessage);
	}
	int expire = (int ) stoi(config->Getvalue("banexpire"));
	ChannelMap chan = Mainframe::instance()->channels();
	ChannelMap::iterator it2 = chan.begin();
	for (; it2 != chan.end(); ++it2) {
		BanSet bans = it2->second->bans();
		BanSet::iterator it3 = bans.begin();
		for (; it3 != bans.end(); ++it3) {
			if ((*it3)->time() + (expire * 60) < now) {
				it2->second->broadcast(":" + config->Getvalue("chanserv") + " MODE " + it2->first + " -b " + (*it3)->mask() + config->EOFMessage);
				Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + it2->first + " -b " + (*it3)->mask());
				it2->second->UnBan(*it3);
			}
		}
		it2->second->resetflood();
	}
	ServerSet::iterator it4 = Servers.begin();
    for(; it4 != Servers.end(); ++it4) {
		if ((*it4)->GetPing() + 240 < now && (*it4)->link() != nullptr) {
			Servidor::sendall("SQUIT " + (*it4)->name());
			Servidor::SQUIT((*it4)->name());
		} else if ((*it4)->GetPing() + 60 < now && (*it4)->link() != nullptr)
			(*it4)->link()->send("PING :" + config->Getvalue("serverName") + config->EOFServer);
	}
	mThrottle.clear();
}

int main(int argc, char *argv[]) {
	bool demonio = true;
	if (argc == 1) {
		std::cout << (Utils::make_string("", "You have started wrong the ircd. For help check: %s -h", argv[0])) << std::endl;
		exit(0);
	}
	for (int i = 1; i < argc; i++) {
		if (boost::iequals(argv[i], "-h") && argc == 2) {
			std::cout << (Utils::make_string("", "Use: %s [-c server.conf] [-p password] -f [-start|-stop|-restart]", argv[0])) << std::endl;
			std::cout << (Utils::make_string("", "Start: %s -start | Stop: %s -stop | Restart: %s -restart", argv[0], argv[0], argv[0])) << std::endl;
			exit(0);
		} if (boost::iequals(argv[i], "-c") && argc > 2) {
			if (access(argv[i+1], W_OK) != 0) {
				std::cout << (Utils::make_string("", "Error loading config file.")) << std::endl;
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
	atexit(exit);
	std::cout << (Utils::make_string("", "My name is: %s", config->Getvalue("serverName").c_str())) << std::endl;
	std::cout << (Utils::make_string("", "Zeus IRC Daemon started")) << std::endl;

	if (ulimit(UL_SETFSIZE, MAX_USERS) < 0) {
		std::cout << "ULIMIT ERROR" << std::endl;
		exit(1);
	} else
		std::cout << "Limite de usuarios configurado a: " << ulimit(UL_GETFSIZE) << std::endl;

	if (demonio == true)
		daemon(1, 0);

	write_pid();

	if (access("zeus.db", W_OK) != 0)
		DB::IniciarDB();

	OperServ::ApplyGlines();

	srand(time(0));

	Config c;

	for (unsigned int i = 0; config->Getvalue("listen["+std::to_string(i)+"]ip").length() > 0; i++) {
		if (config->Getvalue("listen["+std::to_string(i)+"]class") == "client") {
			std::string ip = config->Getvalue("listen["+std::to_string(i)+"]ip");
			int port = (int) stoi(config->Getvalue("listen["+std::to_string(i)+"]port"));
			bool ssl = false;
			if (config->Getvalue("listen["+std::to_string(i)+"]ssl") == "1" || config->Getvalue("listen["+std::to_string(i)+"]ssl") == "true")
				ssl = true;
			bool ipv6 = false;
			boost::thread *t = new boost::thread(boost::bind(&Config::MainSocket, &c, ip, port, ssl, ipv6));
			t->detach();
		} else if (config->Getvalue("listen["+std::to_string(i)+"]class") == "server") {
			std::string ip = config->Getvalue("listen["+std::to_string(i)+"]ip");
			int port = (int) stoi(config->Getvalue("listen["+std::to_string(i)+"]port"));
			bool ssl = false;
			if (config->Getvalue("listen["+std::to_string(i)+"]ssl") == "1" || config->Getvalue("listen["+std::to_string(i)+"]ssl") == "true")
				ssl = true;
			bool ipv6 = false;
			boost::thread *t = new boost::thread(boost::bind(&Config::ServerSocket, &c, ip, port, ssl, ipv6));
			t->detach();
			Servidor::addServer(nullptr, config->Getvalue("serverName"), config->Getvalue("listen["+std::to_string(i)+"]ip"), {});
		}
	}
	for (unsigned int i = 0; config->Getvalue("listen6["+std::to_string(i)+"]ip").length() > 0; i++) {
		if (config->Getvalue("listen6["+std::to_string(i)+"]class") == "client") {
			std::string ip = config->Getvalue("listen6["+std::to_string(i)+"]ip");
			int port = (int) stoi(config->Getvalue("listen6["+std::to_string(i)+"]port"));
			bool ssl = false;
			if (config->Getvalue("listen6["+std::to_string(i)+"]ssl") == "1" || config->Getvalue("listen6["+std::to_string(i)+"]ssl") == "true")
				ssl = true;
			bool ipv6 = true;
			boost::thread *t = new boost::thread(boost::bind(&Config::MainSocket, &c, ip, port, ssl, ipv6));
			t->detach();
		} else if (config->Getvalue("listen6["+std::to_string(i)+"]class") == "server") {
			std::string ip = config->Getvalue("listen6["+std::to_string(i)+"]ip");
			int port = (int) stoi(config->Getvalue("listen6["+std::to_string(i)+"]port"));
			bool ssl = false;
			if (config->Getvalue("listen6["+std::to_string(i)+"]ssl") == "1" || config->Getvalue("listen6["+std::to_string(i)+"]ssl") == "true")
				ssl = true;
			bool ipv6 = true;
			boost::thread *t = new boost::thread(boost::bind(&Config::ServerSocket, &c, ip, port, ssl, ipv6));
			t->detach();
			Servidor::addServer(nullptr, config->Getvalue("serverName"), config->Getvalue("listen6["+std::to_string(i)+"]ip"), {});
		}
	}
	if (config->Getvalue("hub") == config->Getvalue("serverName") && (config->Getvalue("api") == "true" || config->Getvalue("api") == "1"))
		th_api = new boost::thread(api::http);

	while (1) {
		sleep(30);
		timeouts();
	}
	return 0;
}
