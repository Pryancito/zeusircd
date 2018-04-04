#include "mainframe.h"
#include <boost/thread.hpp>

Mainframe *Mainframe::mInstance = NULL;

Mainframe* Mainframe::instance() {
    if(!mInstance) mInstance = new Mainframe();
    return mInstance;
}

Mainframe::~Mainframe() {
    removeAllChannels();
}

void Mainframe::start(std::string ip, int port, bool ssl, bool ipv6) {
    boost::asio::io_service ios;
	Server server(ios, ip, port, ssl, ipv6);
	server.start();
    ios.run();
}

void Mainframe::server(std::string ip, int port, bool ssl, bool ipv6) {
    boost::asio::io_service ios;
	Server server(ios, ip, port, ssl, ipv6);
	server.servidor();
	ios.run();
}

bool Mainframe::doesNicknameExists(const std::string& nick) {
	std::string nickname = nick;
	boost::to_lower(nickname);
    if (mUsers.count(nickname) > 0)
		return true;
	return false;
}

bool Mainframe::addUser(User* user, std::string nick) {
	boost::to_lower(nick);
    if(doesNicknameExists(nick)) return false;
    mUsers[nick] = user;
    return true;
}

bool Mainframe::changeNickname(const std::string& old, const std::string& recent) {
	std::string nickname = recent;
	std::string oldnick = old;
	boost::to_lower(nickname);
	boost::to_lower(oldnick);
    User* tmp = mUsers[oldnick];
    mUsers.erase(oldnick);
    mUsers[nickname] = tmp;
    Servidor::sendall("NICK " + old + " " + recent);
    return true;
}

void Mainframe::removeUser(const std::string& nick) {
	std::string nickname = nick;
	boost::to_lower(nickname);
	mUsers.erase(nickname);
}

User* Mainframe::getUserByName(const std::string& nick) {
	std::string nickname = nick;
	boost::to_lower(nickname);
    if(! doesNicknameExists(nickname) ) return NULL;
    return mUsers[nickname];
}

bool Mainframe::doesChannelExists(const std::string& name) {
	std::string channame = name;
	boost::to_lower(channame);
    return ((mChannels.find(channame)) != mChannels.end());
}

void Mainframe::addChannel(Channel* chan) {
	std::string channame = chan->name();
	boost::to_lower(channame);
    if(!doesChannelExists(channame)) {
        mChannels[channame] = chan;
    }
}

void Mainframe::removeChannel(const std::string& name) { std::string channame = name; boost::to_lower(channame); mChannels.erase(channame); }

Channel* Mainframe::getChannelByName(const std::string& name) {
	std::string channame = name;
	boost::to_lower(channame);
    if(!doesChannelExists(channame))    return NULL;
    return mChannels[channame];
}

void Mainframe::removeAllChannels() {
    ChannelMap::iterator it = mChannels.begin();
    for(; it != mChannels.end(); ++it) {
        delete (it->second);
    }
}

/*void Mainframe::updateChannels() {
    ChannelMap::iterator it = mChannels.begin();
	for (; it != mChannels.end(); ++it) {
        if(it->second->empty()) {
            delete (it->second);
            mChannels.erase(it);
        }    
     }
}*/

ChannelMap Mainframe::channels() const { return mChannels; }

UserMap Mainframe::users() const { return mUsers; }

int Mainframe::countchannels() { return mChannels.size(); }

int Mainframe::countusers() { return mUsers.size(); }