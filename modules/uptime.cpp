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

extern time_t encendido;

class CMD_Uptime : public Module
{
	public:
	CMD_Uptime() : Module("UPTIME", 50, false) {};
	~CMD_Uptime() {};
	virtual void command(User *user, std::string message) override {
		user->SendAsServer("002 " + user->mNickName + " :" + Utils::make_string(user->mLang, "This server started as long as: %s", Utils::Time(encendido).c_str()));
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Uptime);
}
