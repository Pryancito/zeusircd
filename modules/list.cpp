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
 *
 * This file include code from some part of github i can't remember.
*/

#include "ZeusiRCd.h"
#include "module.h"
#include "Utils.h"

class CMD_List : public Module
{
	public:
	CMD_List() : Module("LIST", 50, false) {};
	~CMD_List() {};
	virtual void command(User *user, std::string message) override {
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		if (!user->bSentNick) {
			user->SendAsServer("461 ZeusiRCd :" + Utils::make_string(user->mLang, "You havent used the NICK command yet, you have limited access."));
			return;
		}
		std::string comodin = "*";
		if (results.size() == 2)
			comodin = results[1];
		user->SendAsServer("321 " + user->mNickName + " " + Utils::make_string(user->mLang, "Channel :Users Name"));
		auto it = Channels.begin();
		for (; it != Channels.end(); ++it) {
			std::string mtch = it->second->name;
			std::transform(comodin.begin(), comodin.end(), comodin.begin(), ::tolower);
			std::transform(mtch.begin(), mtch.end(), mtch.begin(), ::tolower);
			if (Utils::Match(comodin.c_str(), mtch.c_str()) == 1)
				user->SendAsServer("322 " + user->mNickName + " " + it->second->name + " " + std::to_string(it->second->users.size()) + " :" + it->second->mTopic);
		}
		user->SendAsServer("323 " + user->mNickName + " :" + Utils::make_string(user->mLang, "End of /LIST"));
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_List);
}
