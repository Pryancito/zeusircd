#include "mainframe.h"
#include "websocket.h"
#include <boost/thread.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>

#define GC_THREADS
#define GC_ALWAYS_MULTITHREADED
#include <gc_cpp.h>
#include <gc.h>


Mainframe *Mainframe::mInstance = nullptr;

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
	Server server(ios, ip, port, ssl, ipv6);
	server.start();
	auto work = boost::make_shared<boost::asio::io_context::work>(ios);
	for (;;) {
		try {
			ios.run();
			break;
		} catch (...) {
			std::cout << "IOS client failure" << std::endl;
			ios.restart();
		}
	}
}

void Mainframe::server(std::string ip, int port, bool ssl, bool ipv6) {
    boost::asio::io_context ios;
	Server server(ios, ip, port, ssl, ipv6);
	server.servidor();
	auto work = boost::make_shared<boost::asio::io_context::work>(ios);
	for (;;) {
		try {
			ios.run();
			break;
		} catch (...) {
			std::cout << "IOS server failure" << std::endl;
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
		} catch (...) {
			std::cout << "IOS websocket failure" << std::endl;
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
