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

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class httpd : public std::enable_shared_from_this<httpd>
{
public:
    httpd(tcp::socket socket)
        : socket_(std::move(socket))
    {
    }

    // Initiate the asynchronous operations associated with the connection.
    void
    start();

	static void
	http_server(tcp::acceptor& acceptor, tcp::socket& socket);
	
private:
    // The socket for the currently connected client.
    tcp::socket socket_;

    // The buffer for performing reads.
    beast::flat_buffer buffer_{8192};

    // The request message.
    http::request<http::dynamic_body> request_;
	
    // The response message.
    http::response<http::dynamic_body> response_;

    // The timer for putting a deadline on connection processing.
    net::steady_timer deadline_{
        socket_.get_executor(), std::chrono::seconds(60)};

    // Asynchronously receive a complete request message.
    void
    read_request();
    
    // Determine what needs to be done with the request message.
    void
    process_request();

    // Construct a response message based on the program state.
    void
    create_response();

    // Asynchronously transmit the response message.
    void
    write_response();

    // Check whether we have spent enough time on this connection.
    void
    check_deadline();
	
	std::string
	parse_request();
};

class Command {
	public:
		static std::string	isreg(const std::vector<std::string> args);
		static std::string	registro(const std::vector<std::string> args);
		static std::string	drop(const std::vector<std::string> args);
		static std::string	auth(const std::vector<std::string> args);
		static std::string	online(const std::vector<std::string> args);
		static std::string	pass(const std::vector<std::string> args);
		static std::string	email(const std::vector<std::string> args);
		static std::string	logs(const std::vector<std::string> args);
		static std::string	ungline(const std::vector<std::string> args);
		static std::string	users(const std::vector<std::string> args);
};
