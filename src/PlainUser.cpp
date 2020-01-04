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

PlainUser::PlainUser(ISocketHandler& h) : TcpSocket(h), deadline(boost::asio::system_executor()) {
    SetLineProtocol();
  }
PlainUser::~PlainUser() {}
  
void PlainUser::OnAccept() {}

void PlainUser::OnLine(const std::string& line) {
	std::vector<std::string> vct;
	Config::split(line, vct, "\n");
	for (unsigned int i = 0; i < vct.size(); i++) {
		vct[i].erase(boost::remove_if(vct[i], boost::is_any_of("\r\n")), vct[i].end());
		Parse(vct[i]);
	}
}

void PlainUser::Send(std::string message) {
	Send(message + "\r\n");
}

void PlainUser::OnDisconnect() {
	Exit(false);
}

std::string PlainUser::ip() {
	return GetRemoteAddress();
}

int PlainUser::Close() {
	CloseAndDelete();
	return 1;
}

/*
void PlainUser::Send(std::string message)
{
	mtx.lock();
	Queue.append(std::move(message + "\r\n"));
	mtx.unlock();
	if (finish == true) {
		finish = false;
		boost::asio::post(Socket.get_executor(), boost::bind(&PlainUser::write, shared_from_this()));
	}
}

void PlainUser::write() {
	if (!Queue.empty()) {
		boost::asio::async_write(Socket, boost::asio::buffer(Queue), boost::bind(&PlainUser::handleWrite, shared_from_this(), _1, _2));
	} else
		finish = true;
}

void PlainUser::handleWrite(const boost::system::error_code& error, std::size_t bytes) {
	mtx.lock();
	Queue.erase(0, bytes);
	mtx.unlock();
	if (error) {
		finish = true;
		return;
	}
	else if (!Queue.empty())
		boost::asio::post(Socket.get_executor(), boost::bind(&PlainUser::write, shared_from_this()));
	else {
		finish = true;
	}
}

void PlainUser::Close()
{
	boost::system::error_code ignored_error;
	Exit(false);
	Socket.cancel(ignored_error);
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
			if (Socket.is_open())
				Send("PING :" + config->Getvalue("serverName"));
			deadline.cancel();
			deadline.expires_from_now(boost::posix_time::seconds(60));
			deadline.async_wait(boost::bind(&PlainUser::check_ping, this, boost::asio::placeholders::error));
		}
	}
}

void PlainUser::read() {
	auto self(shared_from_this());
    boost::asio::async_read_until(Socket, mBuffer, '\n',
        [this, self](boost::system::error_code ec, std::size_t bytes)
        {
          if (!ec)
          {
			std::string message;
			std::istream istream(&mBuffer);
			std::getline(istream, message);
		
            message.erase(boost::remove_if(message, boost::is_any_of("\r\n")), message.end());
			boost::asio::post(Socket.get_executor(), boost::bind(&PlainUser::Parse, shared_from_this(), message));
			
			if (bSendQ + 30 > time(0))
				SendQ += bytes;
			else {
				SendQ = 0;
				bSendQ = time(0);
			}
				
			if (SendQ > 1024*3) {
				quit = true;
				Queue.clear();
				Close();
				return;
			}
			read();
          }
          else
          {
			  quit = true;
            Close();
          }
        });
}

void PlainUser::handleRead(const boost::system::error_code& error, std::size_t bytes) {

}
*/
