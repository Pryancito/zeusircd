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
#pragma once

#include <string>

#include "user.h"
#include "config.h"

class Ircv3 : public gc
{
	private:
		User *mUser;
		std::string version = "302";
		bool negotiating;
		bool usev3;
		bool use_away_notify;
		bool use_uh_in_names;
		bool use_extended_join;
		
	public:
		explicit Ircv3 (User *u) : mUser(u), negotiating(false), usev3(false), use_away_notify(false), use_uh_in_names(false), use_extended_join(false) {
			if (config->Getvalue("ircv3") == "true" || config->Getvalue("ircv3") == "1")
				usev3 = true;
			else
				usev3 = false;
		};
		~Ircv3() {};
		void sendCAP(const std::string &cmd);
		void recvEND();
		void Request(std::string request);
		bool HasCapab(const std::string &capab);
		std::string sts();
};
