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
#include "module.h"
#include "services.h"

class IRCv3 : public Module
{
	public:
	IRCv3() : Module("CAP", 50, false) {};
	~IRCv3() {};
	virtual void command(User *user, std::string message) override {
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		if (results.size() < 2) return;
		else if (results[1] == "LS" || results[1] == "LIST")
			sendCAP(user, results[1]);
		else if (results[1] == "REQ")
			Request(user, message);
		else
			user->negotiating = false;
	}
	void sendCAP(User *user, const std::string &cmd) {
		if (user->negotiating == false) {
			user->negotiating = true;
			user->SendAsServer("CAP * " + cmd + " :away-notify userhost-in-names" + sts(user));
		}
	}

	void Request(User *user, std::string request) {
		if (user->negotiating == true) {
			std::string capabs = ":";
			std::string req = request.substr(9);
			std::vector<std::string>  x;
			Utils::split(req, x, " ");
			for (unsigned int i = 0; i < x.size(); i++) {
				if (x[i] == "away-notify") {
					capabs.append(x[i] + " ");
					user->away_notify = true;
				} else if (x[i] == "userhost-in-names") {
					capabs.append(x[i] + " ");
					user->userhost_in_names = true;
				} else if (x[i] == "extended-join") {
					capabs.append(x[i] + " ");
					user->extended_join = true;
				}
			}
			user->SendAsServer("CAP * ACK " + capabs);
			user->negotiating = false;
		}
	}

	std::string sts(User *user) {
		int puerto = 0;
		if (user->mHost.find(":") != std::string::npos) {
			for (unsigned int i = 0; i < config["listen6"].size(); i++) {
				if (config["listen6"][i]["class"].as<std::string>() == "client" && config["listen6"][i]["ssl"].as<bool>() == true)
					puerto = config["listen6"][i]["port"].as<int>();
			}
		} else {
			for (unsigned int i = 0; i < config["listen"].size(); i++) {
				if (config["listen"][i]["class"].as<std::string>() == "client" && config["listen"][i]["ssl"].as<bool>() == true)
					puerto = config["listen"][i]["port"].as<int>();
			}
		}
		if (puerto == 0)
			return "";
		else
			return " sts=port=" + std::to_string(puerto) + ",duration=10";
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new IRCv3);
}
