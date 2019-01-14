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

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
#include <boost/asio/io_context.hpp>

#include "defines.h"
#include "session.h"

using boost::asio::ip::tcp;
typedef std::map<std::string, unsigned int> 	CloneMap;

class Server {
    public:
   
        Server(boost::asio::io_context& io_context, const std::string &s_ip, int s_port, bool s_ssl, bool s_ipv6);
        Server ();
        ~Server() {};

        void    	start();
        static bool	CheckClone(const std::string &ip);
        static void ThrottleUP(const std::string &ip);
        static bool	HUBExiste();
        void 		servidor();
		static bool CheckDNSBL(const std::string &ip);
		static bool CheckThrottle(const std::string &ip);
		void check_deadline(const std::shared_ptr<Session>& newclient, const boost::system::error_code &e);
    private:
        
        void    startAccept();
        void    handleAccept(const std::shared_ptr<Session>& newclient, const boost::system::error_code& error);
        void	handle_handshake(const std::shared_ptr<Session>& newclient, const boost::system::error_code& error);
        void	Procesar(Session* server);
        tcp::acceptor       mAcceptor;
        std::string ip;
        int port;
        bool ssl;
        bool ipv6;
        boost::asio::deadline_timer deadline;
};
