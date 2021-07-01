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
#include "oper.h"
#include "Utils.h"

class CMD_Oper : public Module
{
	public:
	CMD_Oper() : Module("OPER", 50, false) {};
	~CMD_Oper() {};
	virtual void command(User *user, std::string message) override {
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		Oper oper;
		if (!user->bSentNick) {
			user->SendAsServer("461 ZeusiRCd :" + Utils::make_string(user->mLang, "You havent used the NICK command yet, you have limited access."));
			return;
		} else if (results.size() < 3) {
			user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		} else if (user->getMode('o') == true) {
			user->SendAsServer("381 " + user->mNickName + " :" + Utils::make_string(user->mLang, "You are already an iRCop."));
		} else if (oper.Login(user, results[1], results[2]) == true) {
			user->SendAsServer("381 " + user->mNickName + " :" + Utils::make_string(user->mLang, "Now you are an iRCop."));
		} else {
			user->SendAsServer("481 " + user->mNickName + " :" + Utils::make_string(user->mLang, "Login failed, your attempt has been notified."));
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Oper);
}
