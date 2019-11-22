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

#include "ZeusBaseClass.h"
#include "Config.h"
#include "Utils.h"
#include "sha256.h"
#include "db.h"
#include "services.h"

#include <thread>
#include <set>
#include <iostream>
#include <ulimit.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <ulimit.h>
#include <sys/resource.h>
#include <csignal>
#include <sys/time.h>

extern std::map<std::string, unsigned int> mThrottle;
bool exiting = false;
time_t encendido = time(0);

void write_pid () {
	std::ofstream procid("zeus.pid");
	procid << getpid() << std::endl;
	procid.close();
}
void doexit() {
	if (!exiting) {
		Server::Send("SQUIT " + config->Getvalue("serverName"));
		system("rm -f zeus.pid");
		exiting = true;
		std::cout << "Exiting Zeus." << std::endl;
	}
	exit(0);
}

void sHandler( int signum ) {
	doexit();
}

int main (int argc, char *argv[])
{
	bool demonio = true;

	if (argc == 1) {
		std::cout << (Utils::make_string("", "You have started wrong the ircd. For help check: %s -h", argv[0])) << std::endl;
		doexit();
	}
	for (int i = 1; i < argc; i++) {
		if (strcasecmp(argv[i], "-h") == 0 && argc == 2) {
			std::cout << (Utils::make_string("", "Use: %s [-c server.conf] [-p password] -f [-start|-stop|-restart]", argv[0])) << std::endl;
			std::cout << (Utils::make_string("", "Start: %s -start | Stop: %s -stop | Restart: %s -restart", argv[0], argv[0], argv[0])) << std::endl;
			doexit();
		} if (strcasecmp(argv[i], "-c") == 0 && argc > 2) {
			if (access(argv[i+1], W_OK) != 0) {
				std::cout << (Utils::make_string("", "Error loading config file.")) << std::endl;
				doexit();
			} else {
				config->file = argv[i+1];
				continue;
			}
		} if (strcasecmp(argv[i], "-p") == 0 && argc > 2) {
			std::cout << (Utils::make_string("", "Password %s crypted: %s", argv[i+1], sha256(argv[i+1]).c_str())) << std::endl;
			doexit();
		} if (strcasecmp(argv[i], "-start") == 0) {
			if (access("zeus.pid", W_OK) != 0)
				continue;
			else {
				std::cout << (Utils::make_string("", "The server is already started, if not, delete zeus.pid file.")) << std::endl;
				doexit();
			}
		} else if (strcasecmp(argv[i], "-stop") == 0) {
			if (access("zeus.pid", W_OK) == 0) {
				system("kill -s TERM `cat zeus.pid`");
				system("rm -f zeus.pid");
				doexit();
			} else {
				std::cout << (Utils::make_string("", "The server is not started, if not, stop it manually.")) << std::endl;
				doexit();
			}
		} else if (strcasecmp(argv[i], "-restart") == 0) {
			if (access("zeus.pid", W_OK) == 0)
				system("kill -s TERM `cat zeus.pid`");
			continue;
		} else if (strcasecmp(argv[i], "-f") == 0) {
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
	signal(SIGINT, sHandler);
	signal(SIGTERM, sHandler);

	if (config->Getvalue("dbtype") == "sqlite3")
		system("touch zeus.db");
	
	DB::IniciarDB();

	sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
	DB::SQLiteNoReturn("PRAGMA synchronous = 1;");

	srand(time(0));

	for (unsigned int i = 0; config->Getvalue("listen["+std::to_string(i)+"]ip").length() > 0; i++) {
		if (config->Getvalue("listen["+std::to_string(i)+"]class") == "client") {
			std::string ip = config->Getvalue("listen["+std::to_string(i)+"]ip");
			std::string port = config->Getvalue("listen["+std::to_string(i)+"]port");
			if (config->Getvalue("listen["+std::to_string(i)+"]ssl") == "1" || config->Getvalue("listen["+std::to_string(i)+"]ssl") == "true") {
				std::thread t(&PublicSock::SSListen, ip, port);
				t.detach();
			} else {
				std::thread t(&PublicSock::Listen, ip, port);
				t.detach();
			}
		} else if (config->Getvalue("listen["+std::to_string(i)+"]class") == "server") {
			std::string ip = config->Getvalue("listen["+std::to_string(i)+"]ip");
			std::string port = config->Getvalue("listen["+std::to_string(i)+"]port");
			bool ssl = false;
			if (config->Getvalue("listen["+std::to_string(i)+"]ssl") == "1" || config->Getvalue("listen["+std::to_string(i)+"]ssl") == "true")
				ssl = true;
			std::thread t(&PublicSock::ServerListen, ip, port, ssl);
			t.detach();
		} else if (config->Getvalue("listen["+std::to_string(i)+"]class") == "websocket") {
			std::string ip = config->Getvalue("listen["+std::to_string(i)+"]ip");
			std::string port = config->Getvalue("listen["+std::to_string(i)+"]port");
			std::thread t(&PublicSock::WebListen, ip, port);
			t.detach();
		}
	}
	for (unsigned int i = 0; config->Getvalue("listen6["+std::to_string(i)+"]ip").length() > 0; i++) {
		if (config->Getvalue("listen6["+std::to_string(i)+"]class") == "client") {
			std::string ip = config->Getvalue("listen6["+std::to_string(i)+"]ip");
			std::string port = config->Getvalue("listen6["+std::to_string(i)+"]port");
			if (config->Getvalue("listen6["+std::to_string(i)+"]ssl") == "1" || config->Getvalue("listen6["+std::to_string(i)+"]ssl") == "true") {
				std::thread t(&PublicSock::SSListen, ip, port);
				t.detach();
			} else {
				std::thread t(&PublicSock::Listen, ip, port);
				t.detach();
			}
		} else if (config->Getvalue("listen6["+std::to_string(i)+"]class") == "server") {
			std::string ip = config->Getvalue("listen6["+std::to_string(i)+"]ip");
			std::string port = config->Getvalue("listen6["+std::to_string(i)+"]port");
			bool ssl = false;
			if (config->Getvalue("listen["+std::to_string(i)+"]ssl") == "1" || config->Getvalue("listen["+std::to_string(i)+"]ssl") == "true")
				ssl = true;
			std::thread t(&PublicSock::ServerListen, ip, port, ssl);
			t.detach();
		} else if (config->Getvalue("listen6["+std::to_string(i)+"]class") == "websocket") {
			std::string ip = config->Getvalue("listen6["+std::to_string(i)+"]ip");
			std::string port = config->Getvalue("listen6["+std::to_string(i)+"]port");
			std::thread t(&PublicSock::WebListen, ip, port);
			t.detach();
		}
	}
	
	if (config->Getvalue("hub") == config->Getvalue("serverName") && (config->Getvalue("api") == "true" || config->Getvalue("api") == "1")) {
		std::thread api(boost::bind(&PublicSock::API));
		api.detach();
	}
	
	while (!exiting) {
		sleep(30);
		mThrottle.clear();
		OperServ::ExpireTGline();
	}
	return 0;
}
