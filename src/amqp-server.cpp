/*
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 */
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

#include "amqp.h"
#include "Server.h"
#include "Config.h"

void serveramqp::on_container_start(proton::container &c) {
	listener = c.listen(url);
}

void serveramqp::on_message(proton::delivery &d, proton::message &m) {
	std::string message = proton::get<std::string>(m.body());
	Oper oper;
	std::vector<std::string> vect;
	Config::split(m.reply_to(), vect, "-");
	
	if (vect.size() != 3) {
		d.reject();
		d.connection().close();
		oper.GlobOPs(Utils::make_string("", "The server handshake for %s is incorrect, message has been rejected.", m.reply_to().c_str()));
		return;
	} else if (Server::IsAServer(vect[0]) == false) {
		d.reject();
		d.connection().close();
		oper.GlobOPs(Utils::make_string("", "The server %s is not present into config file.", m.reply_to().c_str()));
		return;
	} else if (vect[1] != config->Getvalue("link-user")) {
		d.reject();
		d.connection().close();
		oper.GlobOPs(Utils::make_string("", "The server handshake for %s is wrong: wrong link-user.", m.reply_to().c_str()));
		return;
	} else if (vect[2] != config->Getvalue("link-pass")) {
		d.reject();
		d.connection().close();
		oper.GlobOPs(Utils::make_string("", "The server handshake for %s is wrong: wrong link-pass.", m.reply_to().c_str()));
		return;
	}
	
	for (Server *srv : Servers) {
		if (srv != nullptr)
			if (srv->ip == vect[0]) {
				if (message == "BURST" && Server::IsConected(srv->ip) == true) {
					Server::SQUIT(srv->name, false, false);
					srv->send("OK");
					Server::sendBurst(srv);
				} else if (message == "BURST") {
					srv->send("OK");
					Server::sendBurst(srv);
				} else if (message == "OK") {
					Server::sendBurst(srv);
				}
				srv->Parse(message);
				d.accept();
				d.connection().close();
				return;
			}
	}
	d.reject();
    d.connection().close();
}
