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
#include "channel.h"
#include "session.h"
#include "services.h"
#include "utils.h"
#include "ircv3.h"
#include "mainframe.h"

#include <iostream>
#include <string>

Channel::Channel(User* creator, const std::string& name, const std::string& topic)
:   mName(name), mTopic(topic), mUsers(),  mOperators(),  mHalfOperators(), mVoices(), flood(0), is_flood(false), mode_r(false), lastflood(0), deadline(channel_user_context)
{
    if(!creator) {
        throw std::runtime_error("Invalid user");
    }

    mUsers.insert(creator);
    if (ChanServ::IsRegistered(mName) == false)
			mOperators.insert(creator);
}

void Channel::addUser(User* user) {
	std::scoped_lock<std::mutex> lock (mtx);
    if(user)
        mUsers.insert(user);
}

void Channel::removeUser(User* user) {
	std::scoped_lock<std::mutex> lock (mtx);
	{
		if (!user) return; 
		if (hasUser(user))  mUsers.erase(user);
		if (isOperator(user)) mOperators.erase(user);
		if (isHalfOperator(user)) mHalfOperators.erase(user);
		if (isVoice(user)) mVoices.erase(user);
	}
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

void Channel::broadcast(const std::string message) {
	std::scoped_lock<std::mutex> lock (mtx);
	{
		UserSet::iterator it = mUsers.begin();
		for (;it != mUsers.end(); it++) {
			if ((*it)->server() == config->Getvalue("serverName"))
				if ((*it)->session())
					(*it)->session()->send(message);
		}
	}
}

void Channel::broadcast_except_me(const std::string nick, const std::string message) {
	std::scoped_lock<std::mutex> lock (mtx);
	{
		UserSet::iterator it = mUsers.begin();
		for (;it != mUsers.end(); it++) {
			if ((*it)->nick() != nick)
				if ((*it)->server() == config->Getvalue("serverName"))
					if ((*it)->session())
						(*it)->session()->send(message);
		}
	}
}

void Channel::broadcast_away(User *user, std::string away, bool on) {
	std::scoped_lock<std::mutex> lock (mtx);
	{
		UserSet::iterator it = mUsers.begin();
		for(; it != mUsers.end(); it++) {
			if ((*it)->server() == config->Getvalue("serverName") && (*it)->session()) {
				if ((*it)->iRCv3()->HasCapab("away-notify") == true && on) {
					(*it)->session()->send(user->messageHeader() + "AWAY " + away + config->EOFMessage);
				} else if ((*it)->iRCv3()->HasCapab("away-notify") == true && !on) {
					(*it)->session()->send(user->messageHeader() + "AWAY" + config->EOFMessage);
				} if (on) {
					(*it)->session()->send(user->messageHeader() + "NOTICE " + name() + " :AWAY ON " + away + config->EOFMessage);
				} else {
					(*it)->session()->send(user->messageHeader() + "NOTICE " + name() + " :AWAY OFF" + config->EOFMessage);
				}
			}
		}
	}
}

void Channel::sendUserList(User* user) {
		bool ircv3 = user->iRCv3()->HasCapab("userhost-in-names");
		std::string names = "";
		std::scoped_lock<std::mutex> lock (mtx);
		{
			UserSet::iterator it = mUsers.begin();
			for(; it != mUsers.end(); it++) {
				std::string nickname = (*it)->nick();
				if (ircv3)
					std::string nickname = (*it)->nick() + "!" + (*it)->ident() + "@" + (*it)->cloak();
				if(isOperator(*it) == true) {
					if (!names.empty())
						names.append(" ");
					names.append("@" + nickname);
				} else if (isHalfOperator(*it) == true) {
					if (!names.empty())
						names.append(" ");
					names.append("%" + nickname);
				} else if (isVoice(*it) == true) {
					if (!names.empty())
						names.append(" ");
					names.append("+" + nickname);
				} else {
					if (!names.empty())
						names.append(" ");
					names.append(nickname);
				}
				if (names.length() > 500) {
					user->session()->sendAsServer("353 "
						+ user->nick() + " = "  + mName + " :" + names +  config->EOFMessage);
					names.clear();
				}
			}
		}
		if (!names.empty())
			user->session()->sendAsServer("353 "
					+ user->nick() + " = "  + mName + " :" + names +  config->EOFMessage);

		user->session()->sendAsServer("366 "
					+ user->nick() + " "  + mName + " :" + Utils::make_string(user->nick(), "End of /NAMES list.")
					+ config->EOFMessage);
}

void Channel::sendWhoList(User* user) {
	std::scoped_lock<std::mutex> lock (mtx);
	{
		UserSet::iterator it = mUsers.begin();
		for(; it != mUsers.end(); ++it) {
			std::string oper = "";
			std::string away = "H";
			if ((*it)->getMode('o') == true)
				oper = "*";
			if ((*it)->is_away() == true)
				away = "G";
			if(isOperator(*it) == true) {
				user->session()->sendAsServer("352 "
					+ (*it)->nick() + " " 
					+ mName + " " 
					+ (*it)->nick() + " " 
					+ (*it)->cloak() + " " 
					+ "*.* " 
					+ (*it)->nick() + " " + away + oper + "@ :0 " 
					+ "ZeusiRCd"
					+ config->EOFMessage);
			} else if(isHalfOperator(*it) == true) {
				user->session()->sendAsServer("352 "
					+ (*it)->nick() + " " 
					+ mName + " " 
					+ (*it)->nick() + " " 
					+ (*it)->cloak() + " " 
					+ "*.* " 
					+ (*it)->nick() + " " + away + oper + "% :0 " 
					+ "ZeusiRCd"
					+ config->EOFMessage);
			} else if(isVoice(*it) == true) {
				user->session()->sendAsServer("352 "
					+ (*it)->nick() + " " 
					+ mName + " " 
					+ (*it)->nick() + " " 
					+ (*it)->cloak() + " " 
					+ "*.* " 
					+ (*it)->nick() + " " + away + oper + "+ :0 " 
					+ "ZeusiRCd"
					+ config->EOFMessage);
			} else {
				user->session()->sendAsServer("352 "
					+ (*it)->nick() + " " 
					+ mName + " " 
					+ (*it)->nick() + " " 
					+ (*it)->cloak() + " " 
					+ "*.* " 
					+ (*it)->nick() + " " + away + oper + " :0 " 
					+ "ZeusiRCd"
					+ config->EOFMessage);
			}
		}
		user->session()->sendAsServer("315 " 
			+ user->nick() + " " 
			+ mName + " :" + "End of /WHO list."
			+ config->EOFMessage);
	}
}

std::string Channel::name() const { return mName; }

std::string Channel::topic() const { return mTopic; }

bool Channel::empty() const { return (mUsers.empty()); }

unsigned int Channel::userCount() const { return mUsers.size(); }

BanSet Channel::bans() {
	return mBans;
}

UserSet Channel::users() {
	return mUsers;
}

bool Channel::IsBan(std::string mask) {
	if (mUsers.size() == 0)
		return false;
	boost::to_lower(mask);
	BanSet bans = mBans;
	BanSet::iterator it = bans.begin();
	for (; it != bans.end(); ++it)
		if (Utils::Match((*it)->mask().c_str(), mask.c_str()) == true)
			return true;
	return false;
}

void Channel::setBan(std::string mask, std::string whois) {
	std::string nombre = name();
	Ban *ban = new Ban(nombre, mask, whois, time(0));
	mBans.insert(ban);
	ban->expire(nombre);
}

void Channel::SBAN(std::string mask, std::string whois, std::string time) {
	time_t tiempo = (time_t ) stoi(time);
	std::string nombre = name();
	Ban *ban = new Ban(nombre, mask, whois, tiempo);
	mBans.insert(ban);
	ban->expire(nombre);
}

void Ban::expire(std::string canal) {
	int expire = (int ) stoi(config->Getvalue("banexpire")) * 60;
	deadline.expires_from_now(boost::posix_time::seconds(expire));
	deadline.async_wait(boost::bind(&Ban::check_expire, this, canal, boost::asio::placeholders::error));
}

void Ban::check_expire(std::string canal, const boost::system::error_code &e) {
	if (!e) {
		Channel* chan = Mainframe::instance()->getChannelByName(canal);
		if (chan) {
			chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -b " + this->mask() + config->EOFMessage);
			Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -b " + this->mask());
			chan->UnBan(this);
		}
	}
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

void Channel::resetflood() {
	flood = 0;
	is_flood = false;
	broadcast(":" + config->Getvalue("chanserv")
		+ " NOTICE "
		+ name() + " :" + Utils::make_string("", "The channel has leaved the flood mode.")
		+ config->EOFMessage);
	Servidor::sendall("NOTICE " + config->Getvalue("chanserv") + " " + name() + " :" + Utils::make_string("", "The channel has leaved the flood mode."));
}

void Channel::increaseflood() {
	if (ChanServ::IsRegistered(mName) == true && ChanServ::HasMode(mName, "FLOOD"))
		flood++;
	else
		return;
	if (flood >= ChanServ::HasMode(mName, "FLOOD") && flood != 0 && is_flood == false) {
		broadcast(":" + config->Getvalue("chanserv")
			+ " NOTICE "
			+ name() + " :" + Utils::make_string("", "The channel has entered into flood mode. The actions are restricted.")
			+ config->EOFMessage);
		Servidor::sendall("NOTICE " + config->Getvalue("chanserv") + " " + name() + " :" + Utils::make_string("", "The channel has entered into flood mode. The actions are restricted."));
		is_flood = true;
	}
	deadline.cancel();
	deadline.expires_from_now(boost::posix_time::seconds(30));
	deadline.async_wait(boost::bind(&Channel::check_flood, this, boost::asio::placeholders::error));
}

bool Channel::isonflood() {
	return is_flood;
}

void Channel::check_flood(const boost::system::error_code &e) {
	if (!e && is_flood == true)
		resetflood();
	else if (!e && is_flood == false)
		flood = 0;
}
