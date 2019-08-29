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

#include "ZeusBaseClass.h"

#include <boost/range/algorithm/remove_if.hpp>
#include <boost/algorithm/string/classification.hpp>

void PlainUser::Send(std::string message)
{
	message.append("\r\n");
	mtx.lock();
	Queue.append(std::move(message));
	mtx.unlock();
	if (finish == true) {
		finish = false;
		write();
	}
}

void PlainUser::write() {
	if (!Queue.empty()) {
		if (Socket.is_open()) {
				boost::asio::async_write(Socket, boost::asio::buffer(Queue, Queue.length()), boost::asio::bind_executor(strand, boost::bind(&PlainUser::handleWrite, shared_from_this(), _1, _2)));
		}
	} else
		finish = true;
}

void PlainUser::handleWrite(const boost::system::error_code& error, std::size_t bytes) {
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

void PlainUser::Close()
{
	if(Socket.is_open()) {
		Exit();
		Socket.cancel();
		Socket.close();
	}
}

std::string PlainUser::ip()
{
	try {
		if (Socket.is_open())
			return Socket.remote_endpoint().address().to_string();
	} catch (boost::system::system_error &e) {
		std::cout << "ERROR getting IP in plain mode" << std::endl;
	}
	return "127.0.0.0";
}

void PlainUser::start()
{
	read();
	deadline.cancel(); 
	deadline.expires_from_now(boost::posix_time::seconds(60)); 
	deadline.async_wait(boost::bind(&PlainUser::check_ping, this, boost::asio::placeholders::error));
	mHost = ip();
}

void PlainUser::check_ping(const boost::system::error_code &e) {
	if (!e) {
		if (bPing + 200 < time(0)) {
			deadline.cancel();
			Close();
		} else {
			deadline.cancel();
			deadline.expires_from_now(boost::posix_time::seconds(60));
			deadline.async_wait(boost::bind(&PlainUser::check_ping, this, boost::asio::placeholders::error));
		}
	}
}

void PlainUser::read() {

	if (Socket.is_open()) {
		boost::asio::async_read_until(Socket, mBuffer, '\n', boost::asio::bind_executor(strand,
			boost::bind(&PlainUser::handleRead, shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred)));
	}
}

void PlainUser::handleRead(const boost::system::error_code& error, std::size_t bytes) {
	if (!error) {
		std::string message;
		std::istream istream(&mBuffer);
		std::getline(istream, message);

		message.erase(boost::remove_if(message, boost::is_any_of("\r\n")), message.end());

		if (message.length() > 1024)
			message.substr(0, 1024);

		boost::asio::post(strand, boost::bind(&PlainUser::Parse, shared_from_this(), message));

		read();
	} else
		Close();
}