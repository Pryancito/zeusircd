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

void serveramqp::on_container_start(proton::container &c) {
	listener = c.listen(url, listen_handler);
}

void serveramqp::on_message(proton::delivery &d, proton::message &m) {
	std::string message = proton::get<std::string>(m.body());
	Oper oper;
	std::cout << "Received: " << message << std::endl;
	
	for (Server *srv : Servers) {
		if (Server::IsAServer(m.reply_to()) == false) {
			oper.GlobOPs(Utils::make_string("", "The server %s is not present into config file.", m.reply_to().c_str()));
			return;
		}
		try
		{
			if (srv->ip == m.reply_to()) {
				if (message == "BURST") {
					Server::sendBurst(srv);
				}
				srv->Parse(message);
			}
		} catch (...) {
			std::cerr << "Error on server parse." << std::endl;
		}
	}
    d.connection().close();
}
