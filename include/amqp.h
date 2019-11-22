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

#pragma once

#include "options.h"
#include "Server.h"

#include <proton/container.hpp>
#include <proton/listen_handler.hpp>
#include <proton/listener.hpp>
#include <proton/message.hpp>
#include <proton/message_id.hpp>
#include <proton/messaging_handler.hpp>
#include <proton/sender.hpp>
#include <proton/sender_options.hpp>
#include <proton/source_options.hpp>
#include <proton/tracker.hpp>
#include <proton/connection.hpp>
#include <proton/delivery.hpp>
#include <proton/receiver_options.hpp>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <cctype>
#include <vector>

extern std::set <Server*> Servers;

class client : public proton::messaging_handler {
    std::string url;
    std::vector<std::string> requests;
    proton::sender sender;
    int sent;
    int confirmed;
    int total;

  public:
    client(const std::string &u, const std::vector<std::string>& r) : url(u), requests(r) {}
    void on_container_start(proton::container &c) override;
	void on_sendable(proton::sender &s) override;
	void on_tracker_accept(proton::tracker &t) override;
	void on_transport_close(proton::transport &) override;
};

class serveramqp : public proton::messaging_handler, public Server {
  private:
    class listener_ready_handler : public proton::listen_handler {
        void on_open(proton::listener& l) override {
            std::cout << "listening on " << l.port() << std::endl;
        }
    };

    typedef std::map<std::string, proton::sender> sender_map;
    listener_ready_handler listen_handler;
    std::string url;
    sender_map senders;
    int address_counter;

  public:
    serveramqp(const std::string &u, std::string ip, std::string port) : Server(ip, port), url(u), address_counter(0) {};
    ~serveramqp() {};
    void on_container_start(proton::container &c) override;
    std::string to_upper(const std::string &s);
    std::string generate_address();
    void on_sender_open(proton::sender &sender) override;
    void on_message(proton::delivery &, proton::message &m) override;
};
