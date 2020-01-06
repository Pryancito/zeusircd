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
#include "Channel.h"
#include "mainframe.h"
#include "Config.h"

#include <boost/range/algorithm/remove_if.hpp>
#include <boost/algorithm/string/classification.hpp>

extern std::mutex quit_mtx;
extern OperSet miRCOps;

  // Returns true if the third party library wants to be notified when the
  // socket is ready for reading.
	bool PlainUser::want_read() const
	{
	return state_ == reading;
	}

	// Notify that third party library that it should perform its read operation.
	void PlainUser::do_read(boost::system::error_code& ec)
	{
		boost::system::error_code error;
		size_t len = boost::asio::read_until(Socket, mBuffer, '\n', error);
        if (!error)
        {
			std::string message;
			std::istream istream(&mBuffer);
			std::getline(istream, message);

			message.erase(boost::remove_if(message, boost::is_any_of("\r\n")), message.end());
			PlainUser::Parse(message);
			
			if (bSendQ + 30 > time(0))
				SendQ += len;
			else {
				SendQ = 0;
				bSendQ = time(0);
			}
				
			if (SendQ > 10000) {
				quit = true;
				Exit(false);
				Socket.close();
				return;
			}
		} else {
			quit = true;
			Exit(false);
			Socket.close();
			return;
		}	
	}

	// Returns true if the third party library wants to be notified when the
	// socket is ready for writing.
	bool PlainUser::want_write() const
	{
	return state_ == writing;
	}

	// Notify that third party library that it should perform its write operation.
	void PlainUser::do_write(boost::system::error_code& ec)
	{
		try
		{
			size_t len = boost::asio::write(Socket, boost::asio::buffer(Queue.front()));
			if (len > 0) {
				Queue.pop_front();
				state_ = reading;
			}
		} catch (boost::system::system_error &e) {
			std::cout << "Error on sending data: " << e.what() << std::endl;
		}
	}

void PlainUser::start_operations()
{
	// Start a read operation if the third party library wants one.
	if (want_read() && !read_in_progress_)
	{
	  read_in_progress_ = true;
	  Socket.async_wait(boost::asio::ip::tcp::socket::wait_read,
		  boost::bind(&PlainUser::handle_read,
			shared_from_this(),
			boost::asio::placeholders::error));
	}

	// Start a write operation if the third party library wants one.
	if (want_write() && !write_in_progress_)
	{
	  write_in_progress_ = true;
	  Socket.async_wait(boost::asio::ip::tcp::socket::wait_write,
		  boost::bind(&PlainUser::handle_write,
			shared_from_this(),
			boost::asio::placeholders::error));
	}
}

void PlainUser::handle_read(boost::system::error_code ec)
{
	read_in_progress_ = false;

	// Notify third party library that it can perform a read.
	if (!ec)
	  do_read(ec);

	// The third party library successfully performed a read on the socket.
	// Start new read or write operations based on what it now wants.
	if (!ec || ec == boost::asio::error::would_block)
	  start_operations();

	// Otherwise, an error occurred. Closing the socket cancels any outstanding
	// asynchronous read or write operations. The connection object will be
	// destroyed automatically once those outstanding operations complete.
	else {
	  Exit(false);
	  Socket.close();
	}
}

void PlainUser::handle_write(boost::system::error_code ec)
{
	write_in_progress_ = false;

	// Notify third party library that it can perform a write.
	if (!ec)
	  do_write(ec);

	// The third party library successfully performed a write on the socket.
	// Start new read or write operations based on what it now wants.
	if (!ec || ec == boost::asio::error::would_block)
	  start_operations();

	// Otherwise, an error occurred. Closing the socket cancels any outstanding
	// asynchronous read or write operations. The connection object will be
	// destroyed automatically once those outstanding operations complete.
	else {
		Exit(false);
		Socket.close();
	}
}

void PlainUser::Send(std::string message)
{
	Queue.push_back(std::move(message + "\r\n"));
	state_ = writing;
	Socket.async_wait(boost::asio::ip::tcp::socket::wait_write,
		  boost::bind(&PlainUser::handle_write,
			shared_from_this(),
			boost::asio::placeholders::error));
}

void PlainUser::Close()
{
	boost::system::error_code ignored_error;
	Exit(false);
	Socket.close(ignored_error);
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
	Socket.non_blocking(true);
	deadline.cancel(); 
	deadline.expires_from_now(boost::posix_time::seconds(60)); 
	deadline.async_wait(boost::bind(&PlainUser::check_ping, this, boost::asio::placeholders::error));
	mHost = ip();
    start_operations();
}

void PlainUser::check_ping(const boost::system::error_code &e) {
	if (!e) {
		if (bPing + 200 < time(0)) {
			deadline.cancel();
			Exit(false);
			Socket.close();
		} else {
			if (Socket.is_open())
				Send("PING :" + config->Getvalue("serverName"));
			deadline.cancel();
			deadline.expires_from_now(boost::posix_time::seconds(60));
			deadline.async_wait(boost::bind(&PlainUser::check_ping, this, boost::asio::placeholders::error));
		}
	}
}
