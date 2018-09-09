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

boost::asio::io_context channel_user_context;

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
	ServerSet::iterator it = Servers.begin();
    for(; it != Servers.end(); ++it) {
		if ((*it)->GetPing() + 240 < now && (*it)->link() != nullptr) {
			Servidor::sendall("SQUIT " + (*it)->name());
			Servidor::SQUIT((*it)->name());
		} else if ((*it)->GetPing() + 60 < now && (*it)->link() != nullptr)
			(*it)->link()->send("PING :" + config->Getvalue("serverName") + config->EOFServer);
	}
	mThrottle.clear();
}

int main(int argc, char *argv[]) {
	bool demonio = true;
	atexit(exit);
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
			std::cout << (Utils::make_string("", "Password %s crypted: %s", argv[i+1], sha256(argv[i+1]).c_str())) << std::endl;
			exit(0);
		} if (boost::iequals(argv[i], "-start")) {
			if (access("zeus.pid", W_OK) != 0)
				continue;
			else {
				std::cout << (Utils::make_string("", "The server is already started, if not, delete zeus.pid file.")) << std::endl;
				exit(0);
			}
		} else if (boost::iequals(argv[i], "-stop")) {
			if (access("zeus.pid", W_OK) == 0) {
				system("kill -s TERM `cat zeus.pid`");
				system("rm -f zeus.pid");
				exit(0);
			} else {
				std::cout << (Utils::make_string("", "The server is not started, if not, stop it manually.")) << std::endl;
				exit(0);
			}
		} else if (boost::iequals(argv[i], "-restart")) {
			if (access("zeus.pid", W_OK) == 0)
				system("kill -s TERM `cat zeus.pid`");
			continue;
		} else if (boost::iequals(argv[i], "-f")) {
			demonio = false;
			continue;
		}
	}

	config->Cargar();

	std::cout << (Utils::make_string("", "My name is: %s", config->Getvalue("serverName").c_str())) << std::endl;
	std::cout << (Utils::make_string("", "Zeus IRC Daemon started")) << std::endl;

	if (ulimit(UL_SETFSIZE, MAX_USERS) < 0) {
		std::cout << "ULIMIT ERROR" << std::endl;
		exit(1);
	} else
		std::cout << (Utils::make_string("", "User limit set to: %s", std::to_string(ulimit(UL_GETFSIZE)).c_str())) << std::endl;

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
		} else if (config->Getvalue("listen["+std::to_string(i)+"]class") == "websocket") {
			std::string ip = config->Getvalue("listen["+std::to_string(i)+"]ip");
			int port = (int) stoi(config->Getvalue("listen["+std::to_string(i)+"]port"));
			bool ssl = false;
			if (config->Getvalue("listen["+std::to_string(i)+"]ssl") == "1" || config->Getvalue("listen["+std::to_string(i)+"]ssl") == "true")
				ssl = true;
			std::cout << "Websocket IPv4 Ready, endpoint => " << ip << ":" << port << " SSL " << (ssl ? "yes" : "no") << std::endl;
			bool ipv6 = false;
			boost::thread *t = new boost::thread(boost::bind(&Config::WebSocket, &c, ip, port, ssl, ipv6));
			t->detach();
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
		} else if (config->Getvalue("listen6["+std::to_string(i)+"]class") == "websocket") {
			std::string ip = config->Getvalue("listen6["+std::to_string(i)+"]ip");
			int port = (int) stoi(config->Getvalue("listen6["+std::to_string(i)+"]port"));
			bool ssl = false;
			if (config->Getvalue("listen6["+std::to_string(i)+"]ssl") == "1" || config->Getvalue("listen6["+std::to_string(i)+"]ssl") == "true")
				ssl = true;
			bool ipv6 = false;
			boost::thread *t = new boost::thread(boost::bind(&Config::WebSocket, &c, ip, port, ssl, ipv6));
			t->detach();
		}
	}
	if (config->Getvalue("hub") == config->Getvalue("serverName") && (config->Getvalue("api") == "true" || config->Getvalue("api") == "1"))
		th_api = new boost::thread(api::http);

	auto work = boost::make_shared<boost::asio::io_context::work>(channel_user_context);
	boost::thread thread(boost::bind(&boost::asio::io_context::run, &channel_user_context));

	while (1) {
		sleep(30);
		timeouts();
	}
	return 0;
}
