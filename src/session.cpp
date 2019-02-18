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
#include "session.h"
#include "parser.h"
#include "websocket.h"
#include "utils.h"

#include <boost/range/algorithm/remove_if.hpp>
#include <boost/algorithm/string/classification.hpp>

#define GC_THREADS
#define GC_ALWAYS_MULTITHREADED
#include <gc_cpp.h>
#include <gc.h>

void Session::start() {
	read();
	deadline.expires_from_now(boost::posix_time::seconds(10));
	deadline.async_wait(boost::bind(&Session::check_deadline, this, boost::asio::placeholders::error));
}

void Session::close() {
	if (websocket == true && wss_.lowest_layer().is_open()) {
		wss_.next_layer().next_layer().cancel();
		//wss_.next_layer().next_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both);
		wss_.next_layer().next_layer().close();
	} else if (ssl == true && mSSL.lowest_layer().is_open()) {
		mSSL.lowest_layer().cancel();
		//mSSL.lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both);
		mSSL.lowest_layer().close();
	} else if (ssl == false && mSocket.is_open()) {
		mSocket.cancel();
		//mSocket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
		mSocket.close();
	}
	deadline.cancel();
}

void Session::check_deadline(const boost::system::error_code &e)
{
	if (!e && mUser.connclose() == true)
		close();
}

void Session::read() {
	if (websocket == true && wss_.lowest_layer().is_open()) {
		wss_.async_read(mBuffer, boost::bind(
										&Session::handleWS, shared_from_this(),
												boost::asio::placeholders::error,
												boost::asio::placeholders::bytes_transferred));
	} else if (ssl == true && mSSL.lowest_layer().is_open()) {
		boost::asio::async_read_until(mSSL, mBuffer, '\n',
                                  boost::bind(&Session::handleRead, shared_from_this(),
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred));
	} else if (ssl == false && mSocket.is_open()) {
		boost::asio::async_read_until(mSocket, mBuffer, '\n',
                                  boost::bind(&Session::handleRead, shared_from_this(),
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred));
	}
}

void Session::handleRead(const boost::system::error_code& error, std::size_t bytes) {
	if (error)
		close();
	else if (bytes == 0)
		close();
	else {
        std::string message;
        std::istream istream(&mBuffer);
        std::getline(istream, message);
        
        message.erase(boost::remove_if(message, boost::is_any_of("\r\n\t")), message.end());

		if (message.length() > 1024)
			message.substr(0, 1024);

        Parser::parse(message, &mUser);
		read();
    }
}

void Session::on_accept(boost::system::error_code ec)
{
	ws_ready = true;
	read();
}

void Session::handleWS(const boost::system::error_code& error, std::size_t bytes) {
	if (ws_ready == false) {
		wss_.async_accept(
            boost::asio::bind_executor(
                strand,
                std::bind(
                    &Session::on_accept,
                    shared_from_this(),
                    std::placeholders::_1)));
	} else if (error)
		close();
	else if (bytes == 0)
		close();
	else {
		std::string message;
        std::istream istream(&mBuffer);
        std::getline(istream, message);
        
        message.erase(boost::remove_if(message, boost::is_any_of("\r\n\t")), message.end());

		if (message.length() > 1024)
			message.substr(0, 1024);

		Parser::parse(message, &mUser);
		read();
    }
}

void Session::send(const std::string message) {
    if (message.length() > 0) {
		boost::system::error_code ignored_error;
		if (websocket == true) {
			if (wss_.next_layer().next_layer().is_open()) {
				std::lock_guard<std::mutex> lock (mtx);
				wss_.write(boost::asio::buffer(message), ignored_error);
			}
		} else if (ssl == true) {
			if (mSSL.lowest_layer().is_open()) {
				std::lock_guard<std::mutex> lock (mtx);
				boost::asio::write(mSSL, boost::asio::buffer(message), boost::asio::transfer_all(), ignored_error);
			}
		} else if (ssl == false) {
			if (mSocket.is_open()) {
				std::lock_guard<std::mutex> lock (mtx);
				boost::asio::write(mSocket, boost::asio::buffer(message), boost::asio::transfer_all(), ignored_error);
			}
		}
	}
}

void Session::sendAsUser(const std::string& message) {
    send(mUser.messageHeader() + message);
}

void Session::sendAsServer(const std::string& message) {
    send(":"+config->Getvalue("serverName")+" " + message);
}

boost::asio::ip::tcp::socket& Session::socket() { return mSocket; }

boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& Session::socket_ssl() { return mSSL; }

boost::beast::websocket::stream<boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>>& Session::socket_wss() { return wss_; }

std::string Session::ip() const {
	if (websocket == true && wss_.lowest_layer().is_open())
		return wss_.lowest_layer().remote_endpoint().address().to_string();
	else if (ssl == true && mSSL.lowest_layer().is_open())
		return mSSL.lowest_layer().remote_endpoint().address().to_string();
	else if (mSocket.is_open())
		return mSocket.remote_endpoint().address().to_string();
	else return "127.0.0.0";
}
