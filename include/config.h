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

#include <cstring>
#include <map>
#include <vector>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/asio/io_context.hpp>
#include <fstream>

class Config
{
        public:
                std::map <std::string, std::string> conf;
                std::string version = "Zeus-4.5";
                std::string file = "zeus.conf";
                std::string EOFMessage = "\r\n";
                std::string EOFServer = "\n";

        void Cargar ();
        void Procesa (std::string linea);
        void Configura (std::string dato, const std::string &valor);
        void DBConfig(std::string dato, std::string uri);
        std::string Getvalue (std::string dato);
        void 	MainSocket(std::string ip, int port, bool ssl, bool ipv6);
        void 	ServerSocket(std::string ip, int port, bool ssl, bool ipv6);
        void 	WebSocket(std::string ip, int port, bool ssl, bool ipv6);
        static void 	Context();
        void API();
};

extern Config *config;
