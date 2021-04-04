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

std::string& ltrim(std::string& str, const std::string& chars = "\t\n\v\f\r ");
std::string& rtrim(std::string& str, const std::string& chars = "\t\n\v\f\r ");
std::string& trim(std::string& str, const std::string& chars = "\t\n\v\f\r ");

class CMD_Away : public Module
{
	public:
	CMD_Away() : Module("AWAY", 49, true) {};
	~CMD_Away() {};
	virtual void command(User *user, std::string message) override {
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		if (!user->bSentNick) {
			user->SendAsServer("461 ZeusiRCd :" + Utils::make_string(user->mLang, "You havent used the NICK command yet, you have limited access."));
			return;
		} else if (results.size() == 1) {
			user->bAway = false;
			for (auto channel : user->channels) {
				if (channel->isonflood() == true && ChanServ::Access(user->mNickName, channel->name) == 0) {
					user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The channel is on flood, you cannot speak."));
					continue;
				}
				channel->increaseflood();
				for (auto *usr : channel->users) {
					if (usr->away_notify == true) {
						usr->deliver(user->messageHeader() + "AWAY");
						usr->deliver(user->messageHeader() + "NOTICE " + channel->name + " :AWAY OFF");
					}
				}
			}
			user->SendAsServer("305 " + user->mNickName + " :AWAY OFF");
			return;
		} else {
			user->bAway = true;
			std::string away = "";
			for (unsigned int i = 1; i < results.size(); ++i) { away.append(results[i] + " "); }
			trim(away);
			for (auto channel : user->channels) {
				if (channel->isonflood() == true && ChanServ::Access(user->mNickName, channel->name) == 0) {
					user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The channel is on flood, you cannot speak."));
					continue;
				}
				channel->increaseflood();
				for (auto *usr : channel->users) {
					if (usr->away_notify == true) {
						usr->deliver(user->messageHeader() + "AWAY " + away);
						usr->deliver(user->messageHeader() + "NOTICE " + channel->name + " :AWAY ON " + away);
					}
				}
			}
			user->SendAsServer("306 " + user->mNickName + " :AWAY ON " + away);
			return;
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Away);
}
