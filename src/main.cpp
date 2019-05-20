/* 
 * This file is part of the ZeusiRCd distribution (https://github.com/Pryancito/zeusircd).
 * Copyright (c) 2019 Rodrigo Santidrian AKA Pryan.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include <iostream>
#include <boost/thread.hpp>
#include <ulimit.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <ulimit.h>
#include <sys/resource.h>
#include <csignal>

#include "config.h"
#include "server.h"
#include "mainframe.h"
#include "sha256.h"
#include "db.h"
#include "services.h"
#include "utils.h"
#include "api.h"
#include "sqlite3.h"
#include "Bayes.h"

#define GC_THREADS
#define GC_ALWAYS_MULTITHREADED
#include <gc_cpp.h>
#include <gc.h>

time_t encendido = time(0);
std::thread *th_api;

using namespace std;
using namespace ourapi;

extern ServerSet Servers;
extern CloneMap mThrottle;

time_t LastbForce = time(0);
ForceMap bForce;
bool exited = false;

boost::asio::io_context channel_user_context;

void write_pid () {
	ofstream procid("zeus.pid");
	procid << getpid() << endl;
	procid.close();
}
void doexit() {
	if (!exited)
		Servidor::sendall("SQUIT " + config->Getvalue("serverName"));
	system("rm -f zeus.pid");
	exited = true;
	exit(0);
}
void sHandler( int signum ) {
	doexit();
}
void timeouts () {
	time_t now = time(0);
	ServerSet::iterator it = Servers.begin();
    for(; it != Servers.end(); ++it) {
		if ((*it)->GetPing() + 120 < now && (*it)->link() != nullptr) {
			Servidor::sendall("SQUIT " + (*it)->name());
			Servidor::SQUIT((*it)->name());
			break;
		} else if ((*it)->GetPing() + 30 < now && (*it)->link() != nullptr)
			(*it)->link()->send("PING " + config->Getvalue("serverName") + config->EOFServer);
	}
	mThrottle.clear();
	if (LastbForce + 3600 < time(0)) {
		bForce.clear();
		LastbForce = time(0);
	}
	delete bayes;
	bayes = new BayesClassifier();
	DB::InitSPAM();
}

int main(int argc, char *argv[]) {
	GC_INIT();
	GC_allow_register_threads ();
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

	struct rlimit limit;
	int max = stoi(config->Getvalue("maxUsers")) * 2;
	limit.rlim_cur = max;
	limit.rlim_max = max;
	if (setrlimit(RLIMIT_NOFILE, &limit) != 0) {
		std::cout << "ULIMIT ERROR" << std::endl;
		exit(1);
	} else
		std::cout << (Utils::make_string("", "User limit set to: %s", config->Getvalue("maxUsers").c_str())) << std::endl;

	if (demonio == true)
		daemon(1, 0);

	struct rlimit core_limits;
	core_limits.rlim_cur = core_limits.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE, &core_limits);

	write_pid();
	atexit(doexit);
	
	signal(SIGTERM, sHandler);
	signal(SIGINT, sHandler);
	
	if (config->Getvalue("dbtype") == "sqlite3")
		system("touch zeus.db");
	
	DB::IniciarDB();

	sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
	DB::SQLiteNoReturn("PRAGMA synchronous = 1;");

	DB::InitSPAM();

	srand(time(0));

	Config c;

	std::thread tim(boost::bind(&Mainframe::timer));
	tim.detach();

	for (unsigned int i = 0; config->Getvalue("listen["+std::to_string(i)+"]ip").length() > 0; i++) {
		if (config->Getvalue("listen["+std::to_string(i)+"]class") == "client") {
			std::string ip = config->Getvalue("listen["+std::to_string(i)+"]ip");
			int port = (int) stoi(config->Getvalue("listen["+std::to_string(i)+"]port"));
			bool ssl = false;
			if (config->Getvalue("listen["+std::to_string(i)+"]ssl") == "1" || config->Getvalue("listen["+std::to_string(i)+"]ssl") == "true")
				ssl = true;
			bool ipv6 = false;
			std::thread t(boost::bind(&Config::MainSocket, &c, ip, port, ssl, ipv6));
			t.detach();
		} else if (config->Getvalue("listen["+std::to_string(i)+"]class") == "server") {
			std::string ip = config->Getvalue("listen["+std::to_string(i)+"]ip");
			int port = (int) stoi(config->Getvalue("listen["+std::to_string(i)+"]port"));
			bool ssl = false;
			if (config->Getvalue("listen["+std::to_string(i)+"]ssl") == "1" || config->Getvalue("listen["+std::to_string(i)+"]ssl") == "true")
				ssl = true;
			bool ipv6 = false;
			std::thread t(boost::bind(&Config::ServerSocket, &c, ip, port, ssl, ipv6));
			t.detach();
			Servidor::addServer(nullptr, config->Getvalue("serverName"), config->Getvalue("listen["+std::to_string(i)+"]ip"), {});
		} else if (config->Getvalue("listen["+std::to_string(i)+"]class") == "websocket") {
			std::string ip = config->Getvalue("listen["+std::to_string(i)+"]ip");
			int port = (int) stoi(config->Getvalue("listen["+std::to_string(i)+"]port"));
			bool ssl = false;
			if (config->Getvalue("listen["+std::to_string(i)+"]ssl") == "1" || config->Getvalue("listen["+std::to_string(i)+"]ssl") == "true")
				ssl = true;
			bool ipv6 = false;
			std::thread t(boost::bind(&Config::WebSocket, &c, ip, port, ssl, ipv6));
			t.detach();
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
			std::thread t(boost::bind(&Config::MainSocket, &c, ip, port, ssl, ipv6));
			t.detach();
		} else if (config->Getvalue("listen6["+std::to_string(i)+"]class") == "server") {
			std::string ip = config->Getvalue("listen6["+std::to_string(i)+"]ip");
			int port = (int) stoi(config->Getvalue("listen6["+std::to_string(i)+"]port"));
			bool ssl = false;
			if (config->Getvalue("listen6["+std::to_string(i)+"]ssl") == "1" || config->Getvalue("listen6["+std::to_string(i)+"]ssl") == "true")
				ssl = true;
			bool ipv6 = true;
			std::thread t(boost::bind(&Config::ServerSocket, &c, ip, port, ssl, ipv6));
			t.detach();
			Servidor::addServer(nullptr, config->Getvalue("serverName"), config->Getvalue("listen6["+std::to_string(i)+"]ip"), {});
		} else if (config->Getvalue("listen6["+std::to_string(i)+"]class") == "websocket") {
			std::string ip = config->Getvalue("listen6["+std::to_string(i)+"]ip");
			int port = (int) stoi(config->Getvalue("listen6["+std::to_string(i)+"]port"));
			bool ssl = false;
			if (config->Getvalue("listen6["+std::to_string(i)+"]ssl") == "1" || config->Getvalue("listen6["+std::to_string(i)+"]ssl") == "true")
				ssl = true;
			bool ipv6 = false;
			std::thread t(boost::bind(&Config::WebSocket, &c, ip, port, ssl, ipv6));
			t.detach();
		}
	}
	if (config->Getvalue("hub") == config->Getvalue("serverName") && (config->Getvalue("api") == "true" || config->Getvalue("api") == "1")) {
		th_api = new std::thread(api::http);
		th_api->detach();
	}

	while (1) {
		sleep(30);
		timeouts();
		GC_gcollect();
	}
	return 0;
}
