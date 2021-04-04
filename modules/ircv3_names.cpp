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

class CMD_Names : public Module
{
	public:
	CMD_Names() : Module("NAMES", 49, true) {};
	~CMD_Names() {};
	virtual void command(User *user, std::string message) override {
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		if (!user->bSentNick) {
			user->SendAsServer("461 ZeusiRCd :" + Utils::make_string(user->mLang, "You havent used the NICK command yet, you have limited access."));
			return;
		}
		else if (results.size() < 2) return;
		Channel* chan = Channel::GetChannel(results[1]);
		if (!chan) {
			user->SendAsServer("403 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The channel doesnt exists."));
			return;
		} else if (!chan->HasUser(user)) {
			user->SendAsServer("441 " + user->mNickName + " :" + Utils::make_string(user->mLang, "You are not into the channel."));
			return;
		} else {
			std::string names = "";
			for (auto *usr : chan->users) {
				std::string nickname = usr->mNickName;
				if (user->userhost_in_names)
					nickname = usr->mNickName + "!" + usr->mIdent + "@" + usr->mvHost;
				if(chan->IsOperator(usr) == true) {
					if (!names.empty())
						names.append(" ");
					names.append("@" + nickname);
				} else if (chan->IsHalfOperator(usr) == true) {
					if (!names.empty())
						names.append(" ");
					names.append("%" + nickname);
				} else if (chan->IsVoice(usr) == true) {
					if (!names.empty())
						names.append(" ");
					names.append("+" + nickname);
				} else {
					if (!names.empty())
						names.append(" ");
					names.append(nickname);
				}
				if (names.length() > 500) {
					user->SendAsServer("353 " + user->mNickName + " = "  + chan->name + " :" + names);
					names.clear();
				}
			}
			if (!names.empty())
				user->SendAsServer("353 " + user->mNickName + " = "  + chan->name + " :" + names);

			user->SendAsServer("366 " + user->mNickName + " "  + chan->name + " :" + Utils::make_string(user->mLang, "End of /NAMES list."));
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Names);
}
