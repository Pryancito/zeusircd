#include "channel.h"
#include "session.h"
#include "services.h"
#include "utils.h"

#include <iostream>
#include <string>

Channel::Channel(User* creator, const std::string& name, const std::string& topic)
:   mName(name), mTopic(topic), mUsers(),  mOperators(),  mHalfOperators(), mVoices(), mPassword(), 
	mSlots(0), bPassword(false), bLimited(false), mode_r(false)
{
    if(!creator) {
        throw std::runtime_error("Invalid user");
    }

    mOperators.insert(creator);
    mUsers.insert(creator);
}

Channel::~Channel() {

}

void Channel::addUser(User* user) {
    if(user) {
        mUsers.insert(user);
        if (!ChanServ::IsRegistered(this->name()))
			mOperators.insert(user);
    }
}

void Channel::removeUser(User* user) {
	if (!user) return; 
	if (hasUser(user))  mUsers.erase(user);
	if (isOperator(user)) mOperators.erase(user);
	if (isHalfOperator(user)) mHalfOperators.erase(user);
	if (isVoice(user)) mVoices.erase(user);
}

bool Channel::hasUser(User* user) { return ((mUsers.find(user)) != mUsers.end()); }

bool Channel::isOperator(User* user) { return ((mOperators.find(user)) != mOperators.end()); }

void Channel::delOperator(User* user) { mOperators.erase(user); }

void Channel::giveOperator(User* user) { mOperators.insert(user); }

bool Channel::isHalfOperator(User* user) { return ((mHalfOperators.find(user)) != mHalfOperators.end()); }

void Channel::delHalfOperator(User* user) { mHalfOperators.erase(user); }

void Channel::giveHalfOperator(User* user) { mHalfOperators.insert(user); }

bool Channel::isVoice(User* user) { return ((mVoices.find(user)) != mVoices.end()); }

void Channel::delVoice(User* user) { mVoices.erase(user); }

void Channel::giveVoice(User* user) { mVoices.insert(user); }

void Channel::broadcast(const std::string& message) {
    UserSet::iterator it = mUsers.begin();
    for(; it != mUsers.end(); ++it) {
		if ((*it)->session() != nullptr)
			(*it)->session()->send(message);
    }
}

void Channel::broadcast_except_me(User* user, const std::string& message) {
    UserSet::iterator it = mUsers.begin();
    for(; it != mUsers.end(); ++it) {
		if ((*it) != user && (*it)->session() != nullptr)
			(*it)->session()->send(message);
    }
}

void Channel::sendUserList(User* user) {
    UserSet::iterator it = mUsers.begin();
    std::string names;
    for(; it != mUsers.end(); ++it) {
        if((mOperators.find((*it))) != mOperators.end()) {
            if (!names.empty())
				names.append(" ");
			names.append("@" + (*it)->nick());
        } else if ((mHalfOperators.find((*it))) != mHalfOperators.end()) {
			if (!names.empty())
				names.append(" ");
			names.append("%" + (*it)->nick());
		} else if ((mVoices.find((*it))) != mVoices.end()) {
			if (!names.empty())
				names.append(" ");
			names.append("+" + (*it)->nick());
		} else {
            if (!names.empty())
				names.append(" ");
			names.append((*it)->nick());
        }
        if (names.length() > 200) {
			user->session()->sendAsServer(ToString(Response::Reply::RPL_NAMREPLY) + " "
				+ user->nick() + " = "  + mName + " :" + names +  config->EOFMessage);
			names.clear();
		}
    }
	if (!names.empty())
		user->session()->sendAsServer(ToString(Response::Reply::RPL_NAMREPLY) + " "
				+ user->nick() + " = "  + mName + " :" + names +  config->EOFMessage);

	user->session()->sendAsServer(ToString(Response::Reply::RPL_ENDOFNAMES) + " "
				+ user->nick() + " "  + mName + " :End of /NAMES list." 
				+ config->EOFMessage);
}

void Channel::sendWhoList(User* user) {
    UserSet::iterator it = mUsers.begin();
    std::string oper = "";
    for(; it != mUsers.end(); ++it) {
		if ((*it)->getMode('o') == true)
			oper = "*";
		else
			oper = "";
        if((mOperators.find((*it))) != mOperators.end()) {
            user->session()->send(":" + config->Getvalue("serverName") + " " 
				+ ToString(Response::Reply::RPL_WHOREPLY) + " " 
				+ (*it)->nick() + " " 
				+ mName + " " 
				+ (*it)->nick() + " " 
				+ (*it)->cloak() + " " 
				+ "*.* " 
				+ (*it)->nick() + " H" + oper + "@ :0 " 
				+ "ZeusiRCd"
				+ config->EOFMessage);
        } else if((mHalfOperators.find((*it))) != mHalfOperators.end()) {
            user->session()->send(":" + config->Getvalue("serverName") + " " 
				+ ToString(Response::Reply::RPL_WHOREPLY) + " " 
				+ (*it)->nick() + " " 
				+ mName + " " 
				+ (*it)->nick() + " " 
				+ (*it)->cloak() + " " 
				+ "*.* " 
				+ (*it)->nick() + " H" + oper + "% :0 " 
				+ "ZeusiRCd"
				+ config->EOFMessage);
        } else if((mVoices.find((*it))) != mVoices.end()) {
            user->session()->send(":" + config->Getvalue("serverName") + " " 
				+ ToString(Response::Reply::RPL_WHOREPLY) + " " 
				+ (*it)->nick() + " " 
				+ mName + " " 
				+ (*it)->nick() + " " 
				+ (*it)->cloak() + " " 
				+ "*.* " 
				+ (*it)->nick() + " H" + oper + "+ :0 " 
				+ "ZeusiRCd"
				+ config->EOFMessage);
        } else {
            user->session()->send(":" + config->Getvalue("serverName") + " " 
				+ ToString(Response::Reply::RPL_WHOREPLY) + " " 
				+ (*it)->nick() + " " 
				+ mName + " " 
				+ (*it)->nick() + " " 
				+ (*it)->cloak() + " " 
				+ "*.* " 
				+ (*it)->nick() + " H" + oper + " :0 " 
				+ "ZeusiRCd"
				+ config->EOFMessage);
        }
    }
	user->session()->sendAsServer(ToString(Response::Reply::RPL_ENDOFWHO) + " " 
		+ user->nick() + " " 
		+ mName + " :End of /WHO list." 
		+ config->EOFMessage);
}

std::string Channel::name() const { return mName; }

std::string Channel::topic() const { return mTopic; }

bool Channel::empty() const { return (mUsers.empty()); }

std::string Channel::password() const { return mPassword; }

unsigned int Channel::userCount() const { return mUsers.size(); }

bool Channel::hasPass() const { return bPassword; }

bool Channel::full() const { return !(mUsers.size() < mSlots); }

bool Channel::limited() const { return bLimited; }

BanSet Channel::bans() {
	return mBans;
}

UserSet Channel::users() {
	return mUsers;
}

bool Channel::IsBan(std::string mask) {
	if (mUsers.size() == 0)
		return false;
	BanSet bans = mBans;
	BanSet::iterator it = bans.begin();
	for (; it != bans.end(); ++it)
		if (Utils::Match(mask.c_str(), (*it)->mask().c_str()) == true)
			return true;
	return false;
}

void Channel::setBan(std::string mask, std::string whois) {
	Ban *ban = new Ban(mask, whois, time(0));
	mBans.insert(ban);
}

void Channel::SBAN(std::string mask, std::string whois, std::string time) {
	time_t tiempo = (time_t ) stoi(time);
	Ban *ban = new Ban(mask, whois, tiempo);
	mBans.insert(ban);
}

std::string Ban::mask() {
	return mascara;
}

std::string Ban::whois() {
	return who;
}

time_t Ban::time() {
	return fecha;
}

void Channel::UnBan(Ban *ban) {
	mBans.erase(ban);
	delete ban;
}

void Channel::cmdOPlus(User *user, User *victim) {
    if (this->hasUser(victim) && this->hasUser(user)) {
        if (this->isOperator(user)) {
            mOperators.insert(victim);
        }
    } else
        user->session()->sendAsServer(ToString(Response::Error::ERR_NOTONCHANNEL) + config->EOFMessage);
}

void Channel::cmdOMinus(User *user) {
    if (this->isOperator(user)) {
        mOperators.erase(user);
    }
}

void Channel::cmdKPlus(const std::string& newPass) {
    bPassword = true;
    mPassword = newPass;
}

void Channel::cmdKMinus() { bPassword = false; }

void Channel::cmdLPlus(const std::string& newNbPlace) {
    bLimited = true;
    std::istringstream iss(newNbPlace);
    iss >> mSlots;
}

void Channel::cmdLMinus() { bLimited = false; }

void Channel::cmdTopic(const std::string& topic) { mTopic = topic; }

bool Channel::getMode(char mode) {
	switch (mode) {
		case 'r': return mode_r;
		default: return false;
	}
	return false;
}

void Channel::setMode(char mode, bool option) {
	switch (mode) {
		case 'r': mode_r = option; break;
		default: break;
	}
	return;
}
