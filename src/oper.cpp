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
#include "oper.h"
#include "sha256.h"
#include "config.h"
#include "utils.h"

#include <string>

OperSet miRCOps;

bool Oper::Login (User* user, const std::string &nickname, const std::string &pass) {
	for (unsigned int i = 0; config->Getvalue("oper["+std::to_string(i)+"]nick").length() > 0; i++)
		if (config->Getvalue("oper["+std::to_string(i)+"]nick") == nickname)
			if (config->Getvalue("oper["+std::to_string(i)+"]pass") == sha256(pass)) {
				miRCOps.insert(user);
				user->sendAsServer("MODE " + user->nick() + " +o" + config->EOFMessage);
				user->setMode('o', true);
				Servidor::sendall("UMODE " + user->nick() + " +o");
				return true;
			}
	GlobOPs(Utils::make_string("", "Failure /oper auth from nick: %s", user->nick().c_str()));
	return false;
}

void Oper::GlobOPs(const std::string &message) {
	OperSet::iterator it = miRCOps.begin();
    for(; it != miRCOps.end(); ++it) {
		if ((*it)->LocalUser == true)
			(*it)->sendAsServer("NOTICE " + (*it)->nick() + " :" + message + config->EOFMessage);
		else
			Servidor::sendall("NOTICE " + config->Getvalue("serverName") + " " + (*it)->nick() + " " + message);
    }
}

int Oper::Count () {
	return miRCOps.size();
}
