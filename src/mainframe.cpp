#include "mainframe.h"
#include "sha256.h"
#include <boost/thread.hpp>

Mainframe *Mainframe::mInstance = nullptr;

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
    if ((mUsers.find(nickname)) != mUsers.end())
		return true;
	return false;
}

bool Mainframe::addUser(User* user, std::string nick) {
	boost::to_lower(nick);
    if(doesNicknameExists(nick)) return false;
    std::string id = sha256(std::to_string(rand()%9999999999)).substr(0, 16);
    user->setid(id);
    user->setNick(nick);
    mUsers[id] = user;
    return true;
}

bool Mainframe::addUser(User* user, std::string nick, std::string ide) {
	boost::to_lower(nick);
    if(doesNicknameExists(nick)) return false;
    user->setid(ide);
    user->setNick(nick);
    mUsers[ide] = user;
    return true;
}

bool Mainframe::changeNickname(const std::string& oid, const std::string& recent) {
    User* tmp = mUsers[oid];
    tmp->setNick(recent);
    mUsers[oid] = tmp;
    Servidor::sendall("NICK " + oid + " " + recent);
    return true;
}

void Mainframe::removeUser(const std::string& id) {
	mUsers.erase(id);
}

User* Mainframe::getUserByName(const std::string& nick) {
    UserMap::iterator it = mUsers.begin();
    for(; it != mUsers.end(); ++it)
		if (boost::iequals(it->second->nick(), nick))
			return (it->second);
	return nullptr;
}

User* Mainframe::getUserByID(const std::string& id) {
	if(!doesNicknameExists(mUsers[id]->nick()))    return nullptr;
    return mUsers[id];
}

bool Mainframe::doesChannelExists(const std::string& name) {
	std::string channame = name;
	boost::to_lower(channame);
    return ((mChannels.find(channame)) != mChannels.end());
}

void Mainframe::addChannel(Channel* chan, std::string channame) {
	boost::to_lower(channame);
    if(!doesChannelExists(channame)) {
		std::string id = sha256(std::to_string(rand()%9999999999)).substr(0, 16);
        mChannels[id] = chan;
    }
}

void Mainframe::addChannel(Channel* chan, std::string channame, std::string ide) {
	boost::to_lower(channame);
    if(!doesChannelExists(channame)) {
        mChannels[ide] = chan;
    }
}

void Mainframe::removeChannel(const std::string& id) { mChannels.erase(id); }

Channel* Mainframe::getChannelByName(const std::string& name) {
    ChannelMap::iterator it = mChannels.begin();
    for(; it != mChannels.end(); ++it)
		if (boost::iequals(it->second->name(), name))
			return (it->second);
	return nullptr;
}

Channel* Mainframe::getChannelByID(const std::string& id) {
    if(!doesChannelExists(mChannels[id]->name()))    return nullptr;
    return mChannels[id];
}

void Mainframe::removeAllChannels() {
    ChannelMap::iterator it = mChannels.begin();
    for(; it != mChannels.end(); ++it) {
        delete (it->second);
    }
}

ChannelMap Mainframe::channels() const { return mChannels; }

UserMap Mainframe::users() const { return mUsers; }

int Mainframe::countchannels() { return mChannels.size(); }

int Mainframe::countusers() { return mUsers.size(); }
