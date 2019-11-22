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
	c.listen(url, listen_handler);
	for (unsigned int i = 0; config->Getvalue("link["+std::to_string(i)+"]ip").length() > 0; i++) {
		Server *srv = new Server(config->Getvalue("link["+std::to_string(i)+"]ip"), config->Getvalue("link["+std::to_string(i)+"]port"));
		Servers.insert(srv);
		Server::sendBurst(srv);
	}
}

std::string serveramqp::to_upper(const std::string &s) {
	std::string uc(s);
	size_t l = uc.size();

	for (size_t i=0; i<l; i++)
		uc[i] = static_cast<char>(std::toupper(uc[i]));

	return uc;
}

std::string serveramqp::generate_address() {
	std::ostringstream addr;
	addr << "server" << address_counter++;

	return addr.str();
}

void serveramqp::on_sender_open(proton::sender &sender) {
	std::cout << "Sender: " << sender.source().address() << std::endl;
	if (sender.source().dynamic()) {
		std::string addr = generate_address();
		sender.open(proton::sender_options().source(proton::source_options().address(addr)));
		senders[addr] = sender;
	}
}

void serveramqp::on_message(proton::delivery &d, proton::message &m) {
	std::string message = proton::get<std::string>(m.body());
	
	std::cout << "Received: " << message << std::endl;
	
	Parse(message);
}
