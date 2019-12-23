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

#include <boost/range/algorithm/remove_if.hpp>
#include <boost/algorithm/string/classification.hpp>

extern std::mutex quit_mtx;
extern OperSet miRCOps;

void LocalWebUser::Send(std::string message)
{
	if (Socket.next_layer().next_layer().is_open() == false)
		return;

	boost::asio::post(
        Socket.get_executor(),
        boost::beast::bind_front_handler(
            &LocalWebUser::on_send,
            shared_from_this(),
            message));
}

void LocalWebUser::on_send(std::string const ss)
{
    queue.push_back(ss);

    if(queue.size() > 1)
        return;

    Socket.async_write(
        boost::asio::buffer(queue.front()),
        boost::beast::bind_front_handler(
            &LocalWebUser::on_write,
            shared_from_this()));
}

void LocalWebUser::on_write(boost::beast::error_code ec, std::size_t)
{
	if (ec)
		Close();

    queue.erase(queue.begin());

    if(!queue.empty())
        Socket.async_write(
            boost::asio::buffer(queue.front()),
            boost::beast::bind_front_handler(
                &LocalWebUser::on_write,
                shared_from_this()));
}

void LocalWebUser::Close()
{
	boost::system::error_code ignored_error;
	Exit();
	Socket.next_layer().next_layer().cancel(ignored_error);
	Socket.next_layer().next_layer().close(ignored_error);
}

std::string LocalWebUser::ip()
{
	try
	{
		return Socket.next_layer().next_layer().remote_endpoint().address().to_string();
	}
	catch (boost::system::system_error &e)
	{
		std::cout << "ERROR getting IP in WSS mode" << std::endl;
	}
	return "127.0.0.0";
}

void LocalWebUser::start()
{
	read();
	deadline.cancel();
	deadline.expires_from_now(boost::posix_time::seconds(60));
	deadline.async_wait(boost::bind(&LocalWebUser::check_ping, this, boost::asio::placeholders::error));
	mHost = ip();
	Socket.binary(true);
}

void LocalWebUser::check_ping(const boost::system::error_code &e)
{
	if (!e)
	{
		if (bPing + 200 < time(0))
		{
			Close();
		}
		else
		{
			LocalWebUser::Send("PING :" + config->Getvalue("serverName"));
			deadline.cancel();
			deadline.expires_from_now(boost::posix_time::seconds(60));
	 		deadline.async_wait(boost::bind(&LocalWebUser::check_ping, this, boost::asio::placeholders::error));
		}
	}
}

void LocalWebUser::read()
{
	if (Socket.next_layer().next_layer().is_open() == true)
		Socket.async_read(mBuffer, boost::beast::bind_front_handler(&LocalWebUser::handleRead, shared_from_this()));
}

void LocalWebUser::on_accept(boost::beast::error_code ec)
{
	if (!ec)
	{
		handshake = true;
		read();
	} else
		Close();
}

void LocalWebUser::handleRead(boost::beast::error_code error, std::size_t bytes)
{
	if (handshake == false)
	{
		Socket.async_accept(
            boost::beast::bind_front_handler(
                    &LocalWebUser::on_accept,
                    shared_from_this()));
	}
	else if (!error)
	{
		std::string message = boost::beast::buffers_to_string(mBuffer.data());

		message.erase(boost::remove_if(message, boost::is_any_of("\r\n")), message.end());

		boost::asio::post(
        Socket.get_executor(),
			boost::beast::bind_front_handler(
				&LocalWebUser::Parse,
				shared_from_this(),
				message));

		mBuffer.consume(mBuffer.size());

		read();
	}
	else
    {
		quit = true;
		Close();
	}
}
