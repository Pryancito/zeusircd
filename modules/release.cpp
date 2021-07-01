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
#include <map>

extern std::map<std::string, unsigned int> bForce;

class CMD_Release : public Module
{
	public:
	CMD_Release() : Module("RELEASE", 50, false) {};
	~CMD_Release() {};
	virtual void command(User *user, std::string message) override {
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		if (!user->bSentNick) {
			user->SendAsServer("461 ZeusiRCd :" + Utils::make_string(user->mLang, "You havent used the NICK command yet, you have limited access."));
			return;
		} else if (results.size() < 2) {
			user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		} else if (user->getMode('o') == false) {
			user->SendAsServer("002 " + user->mNickName + " :" + Utils::make_string(user->mLang, "You do not have iRCop privileges."));
			return;
		} else if (Utils::checknick(results[1]) == false) {
			user->SendAsServer("002 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick contains no-valid characters."));
			return;
		} else if ((bForce.find(results[1])) != bForce.end()) {
			bForce.erase(results[1]);
			user->SendAsServer("002 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick has been released."));
			return;
		} else {
			user->SendAsServer("002 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick isn't in BruteForce lists."));
			return;
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Release);
}
