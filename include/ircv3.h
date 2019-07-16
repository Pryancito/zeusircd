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

#include "config.h"

class User;

class Ircv3
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
		Ircv3 (User *u);
		~Ircv3() {}
		void sendCAP(const std::string &cmd);
		void recvEND();
		void Request(std::string request);
		bool HasCapab(const std::string &capab);
		std::string sts();
};
