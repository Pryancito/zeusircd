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

extern OperSet miRCOps;

class CMD_Opers : public Module
{
	public:
	CMD_Opers() : Module("OPERS", 50, false) {};
	~CMD_Opers() {};
	virtual void command(User *user, std::string message) override {
		for (auto oper : miRCOps) {
			User *usr = User::GetUser(oper);
			if (usr) {
				std::string online = " ( \0033ONLINE\003 )";
				if (usr->bAway)
					online = " ( \0034AWAY\003 )";
				user->SendAsServer("002 " + user->mNickName + " :" + usr->mNickName + online);
			}
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Opers);
}
