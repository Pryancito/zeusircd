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

#include "ZeusiRCd.h"
#include "Config.h"
#include "Utils.h"
#include "sha256.h"
#include "db.h"
#include "services.h"
#include "Server.h"
#include "module.h"

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
std::string OwnAMQP;

void write_pid () {
	std::ofstream procid("zeus.pid");
	procid << getpid() << std::endl;
	procid.close();
}
void doexit() {
	if (!exiting) {
		exiting = true;
		std::cout << "Exiting Zeus." << std::endl;
		//if (config["serverName"])
		//	Server::Send("SQUIT " + config["serverName"].as<std::string>());
		system("rm -f zeus.pid");
		Module::UnloadAll();
		std::cout << "Exited." << std::endl;
		std::_Exit(EXIT_SUCCESS);
	}
}

void sHandler( int signum ) {
	doexit();
}

void PublicSock::Server(std::string ip, std::string port)
{
	auto srv = std::make_shared<Listen>(ip, (int) stoi(port));
	srv->do_accept();
	while (true) { sleep(200); };
}

void PublicSock::SSListen(std::string ip, std::string port)
{
	auto srv = std::make_shared<ListenSSL>(ip, (int) stoi(port));
	srv->start_accept();
	while (true) { sleep(200); };
}

void PublicSock::WebListen(std::string ip, std::string port)
{
	auto srv = std::make_shared<ListenWSS>(ip, (int) stoi(port));
	srv->do_accept();
	while (true) { sleep(200); };
}

void PublicSock::ServerListen(std::string ip, std::string port)
{
	std::string address("amqps://" + ip + ":" + port + "/zeusircd");
	OwnAMQP = ip;
	serveramqp srv(address);
	proton::default_container(srv).run();
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
				config_file = argv[i+1];
				continue;
			}
		} if (strcasecmp(argv[i], "-p") == 0 && argc > 2) {
			std::cout << (Utils::make_string("", "Password %s crypted: %s", argv[i+1], sha256(argv[i+1]).c_str())) << std::endl;
			doexit();
		} if (strcasecmp(argv[i], "-start") == 0) {
			if (access("zeus.pid", W_OK) != 0 && access(config_file.c_str(), W_OK) == 0)
				continue;
			else if (access(config_file.c_str(), W_OK) != 0) {
				std::cout << (Utils::make_string("", "Error loading config file. ( %s ).", config_file.c_str())) << std::endl;
				doexit();
			} else {
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

	config = YAML::LoadFile(config_file);
	
	std::cout << (Utils::make_string("", "My name is: %s", config["serverName"].as<std::string>().c_str())) << std::endl;
	std::cout << (Utils::make_string("", "Zeus IRC Daemon started")) << std::endl;

	struct rlimit limit;
	int max = config["maxUsers"].as<int>() * 2;
	limit.rlim_cur = max;
	limit.rlim_max = max;
	if (setrlimit(RLIMIT_NOFILE, &limit) != 0) {
		std::cout << "ULIMIT ERROR" << std::endl;
		exit(1);
	} else
		std::cout << (Utils::make_string("", "User limit set to: %s", config["maxUsers"].as<std::string>().c_str())) << std::endl;

	int mod = Module::LoadAll();
	if (mod == -1) {
		std::cout << (Utils::make_string("", "Problem loading modules. ircd stopped")) << std::endl;
		exit(1);
	} else
		std::cout << (Utils::make_string("", "Loaded %d modules.", mod)) << std::endl;

	if (demonio == true)
		daemon(1, 0);

	struct rlimit core_limits;
	core_limits.rlim_cur = core_limits.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE, &core_limits);
	
	write_pid();
	atexit(doexit);
	signal(SIGINT, sHandler);
	signal(SIGTERM, sHandler);
	signal(SIGKILL, sHandler);

	if (config["database"]["type"].as<std::string>() == "sqlite3") {
		system("touch zeus.db");
		sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
		DB::SQLiteNoReturn("PRAGMA synchronous = 1;");
	}
	
	DB::IniciarDB();

	srand(time(0));

	for (unsigned int i = 0; i < config["listen"].size(); i++) {
		if (config["listen"][i]["class"].as<std::string>() == "client") {
			std::string ip = config["listen"][i]["ip"].as<std::string>();
			std::string port = config["listen"][i]["port"].as<std::string>();
			if (config["listen"][i]["ssl"].as<bool>() == true) {
				std::thread t(&PublicSock::SSListen, ip, port);
				t.detach();
			} else {
				std::thread t(&PublicSock::Server, ip, port);
				t.detach();
			}
		} else if (config["listen"][i]["class"].as<std::string>() == "server") {
			std::string ip = config["listen"][i]["ip"].as<std::string>();
			std::string port = config["listen"][i]["port"].as<std::string>();
			std::thread t(&PublicSock::ServerListen, ip, port);
			t.detach();
		} else if (config["listen"][i]["class"].as<std::string>() == "websocket") {
			std::string ip = config["listen"][i]["ip"].as<std::string>();
			std::string port = config["listen"][i]["port"].as<std::string>();
			std::thread t(&PublicSock::WebListen, ip, port);
			t.detach();
		}
	}
	for (unsigned int i = 0; i < config["listen6"].size(); i++) {
		if (config["listen6"][i]["class"].as<std::string>() == "client") {
			std::string ip = config["listen6"][i]["ip"].as<std::string>();
			std::string port = config["listen6"][i]["port"].as<std::string>();
			if (config["listen6"][i]["ssl"].as<bool>() == true) {
				std::thread t(&PublicSock::SSListen, ip, port);
				t.detach();
			} else {
				std::thread t(&PublicSock::Server, ip, port);
				t.detach();
			}
		} else if (config["listen6"][i]["class"].as<std::string>() == "server") {
			std::string ip = config["listen6"][i]["ip"].as<std::string>();
			std::string port = config["listen6"][i]["port"].as<std::string>();
			std::thread t(&PublicSock::ServerListen, ip, port);
			t.detach();
		} else if (config["listen6"][i]["class"].as<std::string>() == "websocket") {
			std::string ip = config["listen6"][i]["ip"].as<std::string>();
			std::string port = config["listen6"][i]["port"].as<std::string>();
			std::thread t(&PublicSock::WebListen, ip, port);
			t.detach();
		}
	}
	
	for (unsigned int i = 0; i < config["links"].size(); i++) {
		Server *srv = new Server(config["links"][i]["ip"].as<std::string>(), config["links"][i]["port"].as<std::string>());
		Servers.insert(srv);
		srv->send("BURST");
		Server::sendBurst(srv);
	}
	
	while (!exiting) {
		sleep(30);
		mThrottle.clear();
		OperServ::ExpireTGline();
		Server::ConnectCloud();
	}
	return 0;
}
