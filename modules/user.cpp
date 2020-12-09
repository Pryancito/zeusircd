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
#include "Server.h"

class CMD_User : public Module
{
	public:
	CMD_User() : Module("USER", 50, false) {};
	~CMD_User() {};
	virtual void command(User *user, std::string message) override {
		std::string ident;
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		if (results.size() < 5) {
			user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		} if (results[1].length() > 10)
			ident = results[1].substr(0, 9);
		else
			ident = results[1];
		user->mIdent = ident;
		Server::Send("SUSER " + user->mNickName + " " + ident);
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_User);
}
