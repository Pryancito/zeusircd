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
#include "services.h"
#include "mainframe.h"
#include "db.h"
#include "services.h"
#include "module.h"

#include <string>
#include <iterator>
#include <algorithm>
#include <mutex>
#include <fstream>

extern time_t encendido;
std::map<std::string, unsigned int> bForce;
extern std::set <Server*> Servers;
extern std::mutex log_mtx;

std::string& ltrim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
{
    str.erase(0, str.find_first_not_of(chars));
    return str;
}
 
std::string& rtrim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
{
    str.erase(str.find_last_not_of(chars) + 1);
    return str;
}
 
std::string& trim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
{
    return ltrim(rtrim(str, chars), chars);
}

void User::log(const std::string &message) {
	log_mtx.lock();
	Channel *chan = Mainframe::instance()->getChannelByName("#debug");
	if (config["hub"].as<std::string>() == config["serverName"].as<std::string>()) {
		time_t now = time(0);
		struct tm tm;
		localtime_r(&now, &tm);
		char date[32];
		strftime(date, sizeof(date), "%r %d-%m-%Y", &tm);
		std::ofstream fileLog("ircd.log", std::ios::out | std::ios::app);
		if (fileLog.fail()) {
			if (chan)
				chan->broadcast(":" + config["operserv"].as<std::string>() + " PRIVMSG #debug :Error opening log file.");
			log_mtx.unlock();
			return;
		}
		fileLog << date << " -> " << message << std::endl;
		fileLog.close();
	}
	if (chan) {
		chan->broadcast(":" + config["operserv"].as<std::string>() + " PRIVMSG #debug :" + message);
		Server::Send("PRIVMSG " + config["operserv"].as<std::string>() + " #debug " + message);
	}
	log_mtx.unlock();
}

void LocalUser::Parse(std::string message)
{
	if (quit == true)
		return;
	trim(message);
	std::vector<std::string> results;
	
	Utils::split(message, results, " ");
	if (results.size() == 0)
		return;
	
	std::string cmd = results[0];
	std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
	
	if (cmd == "RELOAD")
	{
		if (getMode('o') == false) {
			SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You do not have iRCop privileges."));
			return;
		} else {
			int unloaded = Module::UnloadAll();
			int loaded = Module::LoadAll();
			if (loaded == -1 || unloaded == -1) {
				SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "Problem loading modules. ircd stopped"));
				exit(1);
			} else
				SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "Unloaded %d modules. Loaded %d modules.", unloaded, loaded));
			return;
		}
	}
	else if (cmd == "REHASH")
	{
		if (getMode('o') == false) {
			SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "You do not have iRCop privileges."));
			return;
		} else {
			config = YAML::LoadFile(config_file);
			SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "The config has been reloaded."));
			return;
		}
	}
	
	bool executed = false;
	std::vector<Widget *> comm;
	for (Widget* w : commands) {
		if (w == nullptr) continue;
		else if (w->cmd == cmd) {
			w->command(this, message);
			executed = true;
			if (w->must_end == true)
				return;
		}
	}
	if (!executed)
		SendAsServer("421 " + mNickName + " :" + Utils::make_string(mLang, "Unknown command.") + " ( " + cmd + " )");
/*
	switch (str2int(cmd.c_str()))
	{
		case str2int("SERVERS"): do_cmd_servers(); break;
		case str2int("CONNECT"): do_cmd_connect(message); break;
		case str2int("SQUIT"): do_cmd_squit(message); break;
		default: SendAsServer("421 " + mNickName + " :" + Utils::make_string(mLang, "Unknown command.")); break;
	}*/
}
/*

void LocalUser::do_cmd_servers() {
	if (getMode('o') == false) {
		SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You do not have iRCop privileges."));
		return;
	} else {
		for (unsigned int i = 0; i < config["links"].size(); i++) {
			std::string ip = config["links"][i]["ip"].as<std::string>();
			std::string port = config["links"][i]["port"].as<std::string>();
			for (Server *server : Servers) {
				if (server->ip == ip) {
					if (Server::IsConected(server->ip) == true) {
						SendAsServer("461 " + mNickName + " :" + server->ip + ":" + port + " " + server->name + " ( \0033CONNECTED\003 )");
						break;
					} else {
						SendAsServer("461 " + mNickName + " :" + ip + " ( \0034DISCONNECTED\003 )");
						break;
					}
				}
			}
		}
	}
}
	
void LocalUser::do_cmd_connect(std::string message) {
	std::vector<std::string> results;
	Utils::split(message, results, " ");
	if (results.size() < 2) {
		SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "More data is needed."));
		return;
	} else if (getMode('o') == false) {
		SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You do not have iRCop privileges."));
		return;
	} else if (Server::IsAServer(results[1]) == false) {
		SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The server is not listed in config."));
		return;
	} else if (Server::IsConected(results[1]) == true) {
		SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The server is already connected."));
		return;
	} else {
		SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "Connecting ..."));
		for (unsigned int i = 0; i < config["links"].size(); i++) {
			if (config["links"][i]["ip"].as<std::string>() == results[1]) {
				Server *srv = new Server(config["links"][i]["ip"].as<std::string>(), config["links"][i]["port"].as<std::string>());
				Servers.insert(srv);
				srv->send("BURST");
				Server::sendBurst(srv);
			}
		}
	}
}
			

void LocalUser::do_cmd_squit(std::string message) {
	std::vector<std::string> results;
	Utils::split(message, results, " ");
	if (results.size() < 2) {
		SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "More data is needed."));
		return;
	} else if (getMode('o') == false) {
		SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You do not have iRCop privileges."));
		return;
	} else if (Server::Exists(results[1]) == false) {
		SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The server is not connected."));
		return;
	} else if (strcasecmp(results[1].c_str(), config["serverName"].as<std::string>().c_str()) == 0) {
		SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You can not make an SQUIT to your own server."));
		return;
	} else {
		Server::SQUIT(results[1], true, true);
		SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The server has been disconnected."));
		return;
	}
}
*/
