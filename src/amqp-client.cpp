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

using proton::receiver_options;
using proton::source_options;

void client::on_container_start(proton::container &c) {
	sender = c.open_sender(url);
	// Create a receiver requesting a dynamically created queue
	// for the message source.
	receiver_options opts = receiver_options().source(source_options().dynamic(true));
	receiver = sender.connection().open_receiver("", opts);
}

void client::send_request() {
	proton::message req;
	req.body(requests.front());
	req.reply_to(receiver.source().address());
	sender.send(req);
}

void client::on_receiver_open(proton::receiver &) {
	send_request();
}

void client::on_message(proton::delivery &d, proton::message &response) {
	if (requests.empty()) return; // Spurious extra message!

	std::cout << requests.front() << " => " << response.body() << std::endl;
	requests.erase(requests.begin());

	if (!requests.empty()) {
		send_request();
	} else {
		d.connection().close();
	}
}
