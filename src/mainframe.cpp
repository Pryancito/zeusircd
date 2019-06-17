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
#include "mainframe.h"
#include "websocket.h"
#include "server.h"

#include <boost/system/error_code.hpp>
#include <boost/make_shared.hpp>
#include <vector>

Mainframe *Mainframe::mInstance = nullptr;
extern boost::asio::io_context channel_user_context;

Mainframe* Mainframe::instance() {
    if(!mInstance) mInstance = new Mainframe();
    return mInstance;
}

Mainframe::~Mainframe() {
    removeAllChannels();
    removeAllUsers();
}

void Mainframe::start(std::string ip, int port, bool ssl, bool ipv6) {
	boost::asio::io_context ios;
	auto work = boost::make_shared<boost::asio::io_context::work>(ios);
	size_t max = std::thread::hardware_concurrency() * 0.75;
	if (max < 1)
		max = 1;
	Server server(max, ios, ip, port, ssl, ipv6);
	server.run();
	server.start();
	for (;;) {
		try {
			ios.run();
			break;
		} catch (std::exception& e) {
			std::cout << "IOS accept failure: " << e.what() << std::endl;
			ios.restart();
		}
	}
}

void Mainframe::server(std::string ip, int port, bool ssl, bool ipv6) {
	boost::asio::io_context ios;
	auto work = boost::make_shared<boost::asio::io_context::work>(ios);
	Server server(ios, ip, port, ssl, ipv6);
	server.run();
	server.servidor();
	for (;;) {
		try {
			ios.run();
			break;
		} catch (std::exception& e) {
			std::cout << "IOS server failure: " << e.what() << std::endl;
			ios.restart();
		}
	}
}

void Mainframe::ws(std::string ip, int port, bool ssl, bool ipv6) {
	boost::asio::io_context ios;
	WebSocket webs(ios, ip, port, ssl, ipv6);
	auto work = boost::make_shared<boost::asio::io_context::work>(ios);
    for (;;) {
		try {
			ios.run();
			break;
		} catch (std::exception& e) {
			std::cout << "IOS websocket failure: " << e.what() << std::endl;
			ios.restart();
		}
	}
}

bool Mainframe::doesNicknameExists(std::string nick) {
	boost::to_lower(nick);
    if ((mUsers.find(nick)) != mUsers.end())
		return true;
	return false;
}

bool Mainframe::addUser(User* user, std::string nick) {
	boost::to_lower(nick);
    if(doesNicknameExists(nick)) return false;
    mUsers[nick] = user;
    return true;
}

bool Mainframe::changeNickname(std::string old, std::string recent) {
	boost::to_lower(recent);
	boost::to_lower(old);
    User* tmp = mUsers[old];
    mUsers.erase(old);
    mUsers[recent] = tmp;
    return true;
}

void Mainframe::removeUser(std::string nick) {
	boost::to_lower(nick);
	mUsers.erase(nick);
}

User* Mainframe::getUserByName(std::string nick) {
	boost::to_lower(nick);
    if(! doesNicknameExists(nick) ) return nullptr;
    return mUsers[nick];
}

bool Mainframe::doesChannelExists(std::string name) {
	boost::to_lower(name);
    return ((mChannels.find(name)) != mChannels.end());
}

void Mainframe::addChannel(Channel* chan) {
	std::string channame = chan->name();
	boost::to_lower(channame);
    if(!doesChannelExists(channame)) {
        mChannels[channame] = chan;
    }
}

void Mainframe::removeChannel(std::string name) { boost::to_lower(name); mChannels.erase(name); }

Channel* Mainframe::getChannelByName(std::string name) {
	boost::to_lower(name);
    if(!doesChannelExists(name))    return nullptr;
    return mChannels[name];
}

void Mainframe::removeAllChannels() {
    ChannelMap::iterator it = mChannels.begin();
    for(; it != mChannels.end(); ++it) {
        delete (it->second);
    }
}

void Mainframe::removeAllUsers() {
    UserMap::iterator it = mUsers.begin();
    for(; it != mUsers.end(); ++it) {
        delete (it->second);
    }
}

ChannelMap Mainframe::channels() const { return mChannels; }

UserMap Mainframe::users() const { return mUsers; }

int Mainframe::countchannels() { return mChannels.size(); }

int Mainframe::countusers() { return mUsers.size(); }

void Mainframe::timer() {
	auto work = boost::make_shared<boost::asio::io_context::work>(channel_user_context);
	for (;;) {
		try {
			channel_user_context.run();
			break;
		} catch (std::exception& e) {
			std::cout << "IOS timer failure: " << e.what() << std::endl;
			channel_user_context.restart();
		}
	}
}
