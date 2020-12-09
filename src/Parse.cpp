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
#include "module.h"
#include "Server.h"
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

void User::Parse(std::string message)
{
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
			Servers.clear();
			for (unsigned int i = 0; i < config["links"].size(); i++) {
				Server *srv = new Server(config["links"][i]["ip"].as<std::string>(), config["links"][i]["port"].as<std::string>());
				Servers.insert(srv);
			}
			
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
}
