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
#include "services.h"
#include "Utils.h"
#include "Server.h"

#include <iostream>
#include <string>

Channel::Channel(LocalUser* creator, const std::string name)
:   mName(name)
, mLocalUsers(),  mLocalOperators(),  mLocalHalfOperators(), mLocalVoices(), mRemoteUsers(),  mRemoteOperators(),  mRemoteHalfOperators(), mRemoteVoices()
, flood(0), is_flood(false), mode_r(false), lastflood(0), deadline(boost::asio::system_executor())
{
    if(!creator) {
        throw std::runtime_error("Invalid user");
    }

    mLocalUsers.insert(creator);
    if (ChanServ::IsRegistered(name) == false)
			mLocalOperators.insert(creator);
}

Channel::Channel(RemoteUser* creator, const std::string name)
:   mName(name)
, mLocalUsers(),  mLocalOperators(),  mLocalHalfOperators(), mLocalVoices(), mRemoteUsers(),  mRemoteOperators(),  mRemoteHalfOperators(), mRemoteVoices()
, flood(0), is_flood(false), mode_r(false), lastflood(0), deadline(boost::asio::system_executor())
{
    if(!creator) {
        throw std::runtime_error("Invalid user");
    }

    mRemoteUsers.insert(creator);
    if (ChanServ::IsRegistered(name) == false)
			mRemoteOperators.insert(creator);
}

void Channel::addUser(LocalUser* user) {
	if (!user)
		return;
	else if (hasUser(user))
		return;
    else
        mLocalUsers.insert(user);
}

void Channel::addUser(RemoteUser* user) {
	if (!user)
		return;
	else if (hasUser(user))
		return;
    else
        mRemoteUsers.insert(user);
}

void Channel::removeUser(LocalUser* user) {
	if (!user) return; 
	if (hasUser(user))  mLocalUsers.erase(user);
	if (isOperator(user)) mLocalOperators.erase(user);
	if (isHalfOperator(user)) mLocalHalfOperators.erase(user);
	if (isVoice(user)) mLocalVoices.erase(user);
	if (userCount() == 0)
		Mainframe::instance()->removeChannel(name());
}

void Channel::removeUser(RemoteUser* user) {
	if (!user) return; 
	if (hasUser(user))  mRemoteUsers.erase(user);
	if (isOperator(user)) mRemoteOperators.erase(user);
	if (isHalfOperator(user)) mRemoteHalfOperators.erase(user);
	if (isVoice(user)) mRemoteVoices.erase(user);
	if (userCount() == 0) 
		Mainframe::instance()->removeChannel(name());
}

bool Channel::hasUser(LocalUser* user) { return (mLocalUsers.find(user)) != mLocalUsers.end(); }

bool Channel::isOperator(LocalUser* user) { return ((mLocalOperators.find(user)) != mLocalOperators.end()); }

void Channel::delOperator(LocalUser* user) { mLocalOperators.erase(user); }

void Channel::giveOperator(LocalUser* user) { mLocalOperators.insert(user); }

bool Channel::isHalfOperator(LocalUser* user) { return ((mLocalHalfOperators.find(user)) != mLocalHalfOperators.end()); }

void Channel::delHalfOperator(LocalUser* user) { mLocalHalfOperators.erase(user); }

void Channel::giveHalfOperator(LocalUser* user) { mLocalHalfOperators.insert(user); }

bool Channel::isVoice(LocalUser* user) { return ((mLocalVoices.find(user)) != mLocalVoices.end()); }

void Channel::delVoice(LocalUser* user) { mLocalVoices.erase(user); }

void Channel::giveVoice(LocalUser* user) { mLocalVoices.insert(user); }

bool Channel::hasUser(RemoteUser* user) { return ((mRemoteUsers.find(user)) != mRemoteUsers.end()); }

bool Channel::isOperator(RemoteUser* user) { return ((mRemoteOperators.find(user)) != mRemoteOperators.end()); }

void Channel::delOperator(RemoteUser* user) { mRemoteOperators.erase(user); }

void Channel::giveOperator(RemoteUser* user) { mRemoteOperators.insert(user); }

bool Channel::isHalfOperator(RemoteUser* user) { return ((mRemoteHalfOperators.find(user)) != mRemoteHalfOperators.end()); }

void Channel::delHalfOperator(RemoteUser* user) { mRemoteHalfOperators.erase(user); }

void Channel::giveHalfOperator(RemoteUser* user) { mRemoteHalfOperators.insert(user); }

bool Channel::isVoice(RemoteUser* user) { return ((mRemoteVoices.find(user)) != mRemoteVoices.end()); }

void Channel::delVoice(RemoteUser* user) { mRemoteVoices.erase(user); }

void Channel::giveVoice(RemoteUser* user) { mRemoteVoices.insert(user); }

void Channel::broadcast(const std::string message) {
	for (LocalUser *user : mLocalUsers) {
		user->Send(message);
	}
}

void Channel::broadcast_except_me(const std::string nick, const std::string message) {
	for (LocalUser *user : mLocalUsers) {
		if (user->mNickName != nick) {
			user->Send(message);
		}
	}
}

void Channel::broadcast_away(LocalUser *user, std::string away, bool on) {
	for (LocalUser *usr : mLocalUsers) {
		if (usr->away_notify == true && on) {
			usr->Send(user->messageHeader() + "AWAY " + away);
		} else if (usr->away_notify == true && !on) {
			usr->Send(user->messageHeader() + "AWAY");
		} if (on) {
			usr->Send(user->messageHeader() + "NOTICE " + name() + " :AWAY ON " + away);
		} else {
			usr->Send(user->messageHeader() + "NOTICE " + name() + " :AWAY OFF");
		}
	}
}

void Channel::sendUserList(LocalUser* user) {
		std::string names = "";
		for (LocalUser *usr : mLocalUsers) {
			std::string nickname = usr->mNickName;
			if (user->userhost_in_names)
				nickname = usr->mNickName + "!" + usr->mIdent + "@" + usr->mvHost;
			if(isOperator(usr) == true) {
				if (!names.empty())
					names.append(" ");
				names.append("@" + nickname);
			} else if (isHalfOperator(usr) == true) {
				if (!names.empty())
					names.append(" ");
				names.append("%" + nickname);
			} else if (isVoice(usr) == true) {
				if (!names.empty())
					names.append(" ");
				names.append("+" + nickname);
			} else {
				if (!names.empty())
					names.append(" ");
				names.append(nickname);
			}
			if (names.length() > 500) {
				user->SendAsServer("353 " + user->mNickName + " = "  + mName + " :" + names);
				names.clear();
			}
		}
		for (RemoteUser *usr : mRemoteUsers) {
			std::string nickname = usr->mNickName;
			if (user->userhost_in_names)
				nickname = usr->mNickName + "!" + usr->mIdent + "@" + usr->mvHost;
			if(isOperator(usr) == true) {
				if (!names.empty())
					names.append(" ");
				names.append("@" + nickname);
			} else if (isHalfOperator(usr) == true) {
				if (!names.empty())
					names.append(" ");
				names.append("%" + nickname);
			} else if (isVoice(usr) == true) {
				if (!names.empty())
					names.append(" ");
				names.append("+" + nickname);
			} else {
				if (!names.empty())
					names.append(" ");
				names.append(nickname);
			}
			if (names.length() > 500) {
				user->SendAsServer("353 " + user->mNickName + " = "  + mName + " :" + names);
				names.clear();
			}
		}
		if (!names.empty())
			user->SendAsServer("353 " + user->mNickName + " = "  + mName + " :" + names);

		user->SendAsServer("366 " + user->mNickName + " "  + mName + " :" + Utils::make_string(user->mLang, "End of /NAMES list."));
}

void Channel::sendWhoList(LocalUser* user) {
	for (LocalUser *usr : mLocalUsers) {
		std::string oper = "";
		std::string away = "H";
		if (usr->getMode('o') == true)
			oper = "*";
		if (usr->bAway == true)
			away = "G";
		if(isOperator(usr) == true) {
			user->SendAsServer("352 "
				+ usr->mNickName + " " 
				+ mName + " " 
				+ usr->mNickName + " " 
				+ usr->mvHost + " " 
				+ "*.* " 
				+ usr->mNickName + " " + away + oper + "@ :0 " 
				+ "ZeusiRCd");
		} else if(isHalfOperator(usr) == true) {
			user->SendAsServer("352 "
				+ usr->mNickName + " " 
				+ mName + " " 
				+ usr->mNickName + " " 
				+ usr->mvHost + " " 
				+ "*.* " 
				+ usr->mNickName + " " + away + oper + "% :0 " 
				+ "ZeusiRCd");
		} else if(isVoice(usr) == true) {
			user->SendAsServer("352 "
				+ usr->mNickName + " " 
				+ mName + " " 
				+ usr->mNickName + " " 
				+ usr->mvHost + " " 
				+ "*.* " 
				+ usr->mNickName + " " + away + oper + "+ :0 " 
				+ "ZeusiRCd");
		} else {
			user->SendAsServer("352 "
				+ usr->mNickName + " " 
				+ mName + " " 
				+ usr->mNickName + " " 
				+ usr->mvHost + " " 
				+ "*.* " 
				+ usr->mNickName + " " + away + oper + " :0 " 
				+ "ZeusiRCd");
		}
	}
	for (RemoteUser *usr : mRemoteUsers) {
		std::string oper = "";
		std::string away = "H";
		if (usr->getMode('o') == true)
			oper = "*";
		if (usr->bAway == true)
			away = "G";
		if(isOperator(usr) == true) {
			user->SendAsServer("352 "
				+ usr->mNickName + " " 
				+ mName + " " 
				+ usr->mNickName + " " 
				+ usr->mvHost + " " 
				+ "*.* " 
				+ usr->mNickName + " " + away + oper + "@ :0 " 
				+ "ZeusiRCd");
		} else if(isHalfOperator(usr) == true) {
			user->SendAsServer("352 "
				+ usr->mNickName + " " 
				+ mName + " " 
				+ usr->mNickName + " " 
				+ usr->mvHost + " " 
				+ "*.* " 
				+ usr->mNickName + " " + away + oper + "% :0 " 
				+ "ZeusiRCd");
		} else if(isVoice(usr) == true) {
			user->SendAsServer("352 "
				+ usr->mNickName + " " 
				+ mName + " " 
				+ usr->mNickName + " " 
				+ usr->mvHost + " " 
				+ "*.* " 
				+ usr->mNickName + " " + away + oper + "+ :0 " 
				+ "ZeusiRCd");
		} else {
			user->SendAsServer("352 "
				+ usr->mNickName + " " 
				+ mName + " " 
				+ usr->mNickName + " " 
				+ usr->mvHost + " " 
				+ "*.* " 
				+ usr->mNickName + " " + away + oper + " :0 " 
				+ "ZeusiRCd");
		}
	}
	user->SendAsServer("315 " 
		+ user->mNickName + " " 
		+ mName + " :" + "End of /WHO list.");
}

std::string Channel::name() const { return mName; }

std::string Channel::topic() const { return mTopic; }

size_t Channel::userCount() const { return (mLocalUsers.size() + mRemoteUsers.size()); }

BanSet Channel::bans() {
	return mBans;
}

pBanSet Channel::pbans() {
	return pBans;
}

bool Channel::IsBan(std::string mask) {
	if (userCount() == 0)
		return false;
	std::transform(mask.begin(), mask.end(), mask.begin(), ::tolower);
	for (auto ban : mBans)
		if (Utils::Match(ban->mask().c_str(), mask.c_str()) == true)
			return true;
	for (auto ban : pBans)
		if (Utils::Match(ban->mask().c_str(), mask.c_str()) == true)
			return true;
	return false;
}

void Channel::setBan(std::string mask, std::string whois) {
	std::string nombre = name();
	Ban *ban = new Ban(nombre, mask, whois, time(0));
	mBans.insert(ban);
}

void Channel::setpBan(std::string mask, std::string whois) {
	std::string nombre = name();
	pBan *ban = new pBan(nombre, mask, whois, time(0));
	pBans.insert(ban);
}

void Channel::SBAN(std::string mask, std::string whois, std::string time) {
	time_t tiempo = (time_t ) stoi(time);
	std::string nombre = name();
	Ban *ban = new Ban(nombre, mask, whois, tiempo);
	mBans.insert(ban);
}

void Channel::SPBAN(std::string mask, std::string whois, std::string time) {
	time_t tiempo = (time_t ) stoi(time);
	std::string nombre = name();
	pBan *ban = new pBan(nombre, mask, whois, tiempo);
	pBans.insert(ban);
}

void Ban::ExpireBan(const boost::system::error_code &e) {
	if (!e)
	{
		Channel* chan = Mainframe::instance()->getChannelByName(canal);
		if (chan) {
			chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -b " + this->mask());
			Server::Send("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -b " + this->mask());
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

std::string pBan::mask() {
	return mascara;
}

std::string pBan::whois() {
	return who;
}

time_t pBan::time() {
	return fecha;
}

void Channel::UnpBan(pBan *ban) {
	pBans.erase(ban);
	delete ban;
}

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
		+ name() + " :" + Utils::make_string("", "The channel has leaved the flood mode."));
	Server::Send("NOTICE " + config->Getvalue("chanserv") + " " + name() + " :" + Utils::make_string("", "The channel has leaved the flood mode."));
}

void Channel::increaseflood() {
	if (ChanServ::IsRegistered(mName) == true && ChanServ::HasMode(mName, "FLOOD"))
		flood++;
	else
		return;
	if (flood >= ChanServ::HasMode(mName, "FLOOD") && flood != 0 && is_flood == false) {
		broadcast(":" + config->Getvalue("chanserv")
			+ " NOTICE "
			+ name() + " :" + Utils::make_string("", "The channel has entered into flood mode. The actions are restricted."));
		Server::Send("NOTICE " + config->Getvalue("chanserv") + " " + name() + " :" + Utils::make_string("", "The channel has entered into flood mode. The actions are restricted."));
		is_flood = true;
	}
}

bool Channel::isonflood() {
	return is_flood;
}

void Channel::ExpireFlood() {
	if (is_flood == true)
		resetflood();
	else
		flood = 0;
}
