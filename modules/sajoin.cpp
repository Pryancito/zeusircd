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
#include "Utils.h"
#include "Server.h"
#include "services.h"

class CMD_Sajoin : public Module
{
	public:
	CMD_Sajoin() : Module("SAJOIN", 50, false) {};
	~CMD_Sajoin() {};
	virtual void command(User *user, std::string message) override {
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		if (user->getMode('o') == false) {
			user->deliver(":" + config["operserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "To make this action, you need identify first."));
			return;
		}
		else if (results.size() < 3) {
			user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		}
		else if (User::FindUser(results[1]) == false) {
			user->SendAsServer("401 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick does not exist."));
			return;
		}
		std::vector<std::string>  x;
		Utils::split(results[2], x, ",");
		User *usr = User::GetUser(results[1]);
		for (unsigned int i = 0; i < x.size(); i++) {
			if (Utils::checkchan(x[i]) == false) {
				user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The channel contains no-valid characters."));
				continue;
			} else if (x[i].size() < 2) {
				user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The channel name is empty."));
				continue;
			} else if (x[i].length() > config["chanlen"].as<unsigned int>()) {
				user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The channel name is too long."));
				continue;
			} else if (ChanServ::IsRegistered(x[i]) == false) {
				Channel* chan = Channel::GetChannel(x[i]);
				if (chan) {
					if (chan->HasUser(usr) == true) {
						user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The user is already on this channel."));
						continue;
					}
					chan->join(usr);
					usr->SendAsServer("461 " + usr->mNickName + " :" + Utils::make_string(usr->mLang, "You were forced to join %s.", chan->name.c_str()));
					Server::Send("SJOIN " + usr->mNickName + " " + chan->name + " +x");
					continue;
				} else {
					Channel *chan = new Channel(x[i]);
					if (chan) {
						std::transform(x[i].begin(), x[i].end(), x[i].begin(), ::tolower);
						Channels.insert(std::pair<std::string,Channel *>(x[i],chan));
						chan->GiveOperator(usr);
						chan->join(usr);
						usr->SendAsServer("461 " + usr->mNickName + " :" + Utils::make_string(usr->mLang, "You were forced to join %s.", chan->name.c_str()));
						Server::Send("SJOIN " + usr->mNickName + " " + chan->name + " +o");
						continue;
					}
				}
			} else {
				Channel* chan = Channel::GetChannel(x[i]);
				if (chan) {
					if (chan->HasUser(usr) == true) {
						user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The user is already on this channel."));
						continue;
					} else {
						chan->join(usr);
						usr->SendAsServer("461 " + usr->mNickName + " :" + Utils::make_string(usr->mLang, "You were forced to join %s.", chan->name.c_str()));
						ChanServ::DoRegister(usr, chan);
						ChanServ::CheckModes(usr, chan->name);
						continue;
					}
				} else {
					chan = new Channel(x[i]);
					if (chan) {
						std::transform(x[i].begin(), x[i].end(), x[i].begin(), ::tolower);
						Channels.insert(std::pair<std::string,Channel *>(x[i],chan));
						chan->join(usr);
						usr->SendAsServer("461 " + usr->mNickName + " :" + Utils::make_string(usr->mLang, "You were forced to join %s.", chan->name.c_str()));
						ChanServ::DoRegister(usr, chan);
						ChanServ::CheckModes(usr, chan->name);
						continue;
					}
				}
			}
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Sajoin);
}
