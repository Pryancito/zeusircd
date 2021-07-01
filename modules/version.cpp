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
#include "Config.h"
#include "db.h"

class CMD_Version : public Module
{
	public:
	CMD_Version() : Module("VERSION", 50, false) {};
	~CMD_Version() {};
	virtual void command(User *user, std::string message) override {
		user->SendAsServer("002 " + user->mNickName + " :" + Utils::make_string(user->mLang, "Version of ZeusiRCd: %s", config["version"].as<std::string>().c_str()));
		if (user->getMode('o') == true)
			user->SendAsServer("002 " + user->mNickName + " :" + Utils::make_string(user->mLang, "Version of DataBase: %s", DB::GetLastRecord().c_str()));
		}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Version);
}
