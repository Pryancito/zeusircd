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

void Session::start() {
	read();
	deadline_s.expires_from_now(boost::posix_time::seconds(10));
	deadline_s.async_wait(boost::bind(&Session::check_deadline, this, boost::asio::placeholders::error));
}

void Session::close() {
	boost::system::error_code ignored_error;
	if (websocket == true) {
		Exit();
		get_lowest_layer(wss_).socket().cancel(ignored_error);
		get_lowest_layer(wss_).socket().close(ignored_error);
	} else if (ssl == true) {
		if (mSSL.lowest_layer().is_open()) {
			Exit();
			mSSL.lowest_layer().cancel(ignored_error);
			mSSL.lowest_layer().close(ignored_error);
		}
	} else {
		if(mSocket.is_open()) {
			Exit();
			mSocket.cancel(ignored_error);
			mSocket.close(ignored_error);
		}
	}
	deadline_s.cancel();
}

void Session::check_deadline(const boost::system::error_code &e)
{
	if (!e && this->connclose() == true)
		close();
}

void Session::read() {
	if (websocket == true) {
		if (get_lowest_layer(wss_).socket().is_open()) {
			wss_.async_read(mBuffer, boost::asio::bind_executor(strand,
				boost::bind(&Session::handleWS, shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred)));
		}
	} else if (ssl == true) {
		if (mSSL.lowest_layer().is_open()) {
			boost::asio::async_read_until(mSSL, mBuffer, '\n', boost::asio::bind_executor(strand,
				boost::bind(&Session::handleRead, shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred)));
		}
	} else if (ssl == false) {
		if (mSocket.is_open()) {
			boost::asio::async_read_until(mSocket, mBuffer, '\n', boost::asio::bind_executor(strand,
				boost::bind(&Session::handleRead, shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred)));
		}
	}
}

void Session::handleRead(const boost::system::error_code& error, std::size_t bytes) {
	if (!error) {
		std::string message;
		std::istream istream(&mBuffer);
		std::getline(istream, message);

		message.erase(boost::remove_if(message, boost::is_any_of("\r\n")), message.end());
		
		if (message.length() > 1024)
			message.substr(0, 1024);

		if (!bIsQuit)
			boost::asio::post(strand, boost::bind(&Parser::parse, message, mUser()));
		
		if (message.substr(0, 4) == "QUIT")
			bIsQuit = true;
		
		read();
	} else
		close();
}

void Session::on_accept(boost::system::error_code ec)
{
	if (!ec) {
		ws_ready = true;
		read();
	} else
		close();
}

void Session::handleWS(const boost::system::error_code& error, std::size_t bytes) {
	if (ws_ready == false) {
		wss_.async_accept(
		boost::bind(
			&Session::on_accept,
			shared_from_this(),
			boost::asio::placeholders::error));
	} else if (!error) {
		std::string message;
		std::istream istream(&mBuffer);
		std::getline(istream, message);

		message.erase(boost::remove_if(message, boost::is_any_of("\r\n")), message.end());

		if (message.length() > 1024)
			message.substr(0, 1024);

		if (!bIsQuit)
			boost::asio::post(strand, boost::bind(&Parser::parse, message, mUser()));
		
		if (message.substr(0, 4) == "QUIT")
			bIsQuit = true;
		
		read();
	}
}

void Session::write() {
	if (!Queue.empty()) {
		if (ssl == true) {
			if (mSSL.lowest_layer().is_open()) {
				boost::asio::async_write(mSSL, boost::asio::buffer(Queue, Queue.length()), boost::asio::bind_executor(strand, boost::bind(&Session::handleWrite, shared_from_this(), _1, _2)));
			}
		} else {
			if (mSocket.is_open()) {
					boost::asio::async_write(mSocket, boost::asio::buffer(Queue, Queue.length()), boost::asio::bind_executor(strand, boost::bind(&Session::handleWrite, shared_from_this(), _1, _2)));
			}
		}
	} else
		finish = true;
}

void Session::send(const std::string message) {
	if (websocket == true) {
		if (get_lowest_layer(wss_).socket().is_open()) {
				wss_.write(boost::asio::buffer(message, message.length()));
		}
	} else {
		mtx.lock();
		Queue.append(std::move(message));
		mtx.unlock();
		if (finish == true) {
			finish = false;
			boost::asio::post(strand, boost::bind(&Session::write, this));
		}
	}
}

void Session::handleWrite(const boost::system::error_code& error, std::size_t bytes) {
	if (error) {
		Queue.clear();
		finish = true;
		return;
	}
	mtx.lock();
	Queue.erase(0, bytes);
	mtx.unlock();
	if (!Queue.empty())
		write();
	else {
		finish = true;
		Queue.clear();
	}
}

void Session::sendAsUser(const std::string message) {
	send(messageHeader() + message);
}

void Session::sendAsServer(const std::string message) {
	send(":"+config->Getvalue("serverName")+" " + message);
}

boost::asio::ip::tcp::socket& Session::socket() { return mSocket; }

boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& Session::socket_ssl() { return mSSL; }

boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>& Session::socket_wss() { return wss_; }

std::string Session::ip() const {
	if (websocket == true) {
		if (get_lowest_layer(wss_).socket().is_open()) {
			return wss_.next_layer().next_layer().socket().remote_endpoint().address().to_string();
		}
	} else if (ssl == true) {
		if (mSSL.lowest_layer().is_open()) {
			return mSSL.lowest_layer().remote_endpoint().address().to_string();
		}
	} else if (mSocket.is_open()) {
		return mSocket.remote_endpoint().address().to_string();
	}
	return "127.0.0.0";
}
