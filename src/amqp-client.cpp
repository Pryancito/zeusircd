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

void client::on_container_start(proton::container& c) {
	c.connect(url);
}
void client::on_connection_open(proton::connection& c) {
	c.open_sender("zeusircd");
}

void client::on_sendable(proton::sender &s) {
	proton::message msg(queue);

	std::string reply(OwnAMQP + "-" + config->Getvalue("link-user") + "-" + config->Getvalue("link-pass"));

	msg.reply_to(reply);

	s.send(msg);
	s.close();
}

void client::on_tracker_accept(proton::tracker &t) {
	t.connection().close();
}

void client::on_sender_error(proton::sender &s) {
	for (Server *srv : Servers) {
		if (srv != nullptr) {
			if (srv->ip == ip) {
				Oper oper;
				oper.GlobOPs(Utils::make_string("", "Sending Error to %s. Closing connection.", ip.c_str()));
				Server::SQUIT(srv->name, true, false);
				return;
			}
		}
	}
}

void client::on_error (const proton::error_condition &c) {
//	Oper oper;
//	oper.GlobOPs(Utils::make_string("", "Sending Error: %s", c.what().c_str()));
}
