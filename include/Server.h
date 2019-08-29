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
#include <vector>
#include <deque>
#include <iostream>

#include <proton/connection.hpp>
#include <proton/container.hpp>
#include <proton/delivery.hpp>
#include <proton/message.hpp>
#include <proton/message_id.hpp>
#include <proton/messaging_handler.hpp>
#include <proton/link.hpp>
#include <proton/listen_handler.hpp>
#include <proton/listener.hpp>
#include <proton/sender.hpp>
#include <proton/sender_options.hpp>
#include <proton/source_options.hpp>
#include <proton/value.hpp>
#include <proton/tracker.hpp>
#include <proton/types.hpp>

class Server : public proton::messaging_handler {
	public:
		class listener_ready_handler : public proton::listen_handler {
			void on_open(proton::listener& l) override {}
		};
		Server(const std::string &s) :
        url(s), expected(0), received(0), sent(0), confirmed(0), total(0), address_counter(0) {}
		~Server() {};
		static bool CanConnect(const std::string ip);
		static bool CheckClone(const std::string &ip);
		static bool CheckThrottle(const std::string &ip);
		static void ThrottleUP(const std::string &ip);
		static bool HUBExiste();
		static void sendall(const std::string message);
		
		void sendBurst();
		void on_container_start(proton::container &c) override;
		void on_message(proton::delivery &d, proton::message &msg) override;
		void on_tracker_accept(proton::tracker &t) override;
		void on_transport_close(proton::transport &) override;
		void on_sender_open(proton::sender &sender) override;
		void Send(std::string message);
		
        std::vector <std::string> servers;
		std::deque <std::string> Queue;
		std::string url;
		std::map<std::string, proton::sender> senders;
		proton::listener listener;
		listener_ready_handler listen_handler;
		int expected;
		int received;
		int sent;
		int confirmed;
		int total;
		int address_counter;
		bool burst = false;
};
