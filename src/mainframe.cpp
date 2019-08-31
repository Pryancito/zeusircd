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

Mainframe *Mainframe::mInstance = nullptr;

Mainframe* Mainframe::instance() {
    if(!mInstance) mInstance = new Mainframe();
    return mInstance;
}

Mainframe::~Mainframe() {
    removeAllChannels();
    removeAllUsers();
}

bool Mainframe::doesNicknameExists(std::string nick) {
	std::transform(nick.begin(), nick.end(), nick.begin(), ::tolower);
    if ((mLocalUsers.find(nick)) != mLocalUsers.end())
		return true;
    if ((mRemoteUsers.find(nick)) != mRemoteUsers.end())
		return true;
	return false;
}

bool Mainframe::addLocalUser(LocalUser* user, std::string nick) {
	std::transform(nick.begin(), nick.end(), nick.begin(), ::tolower);
    if(doesNicknameExists(nick)) return false;
    mLocalUsers[nick] = user;
    return true;
}

bool Mainframe::addRemoteUser(RemoteUser* user, std::string nick) {
	std::transform(nick.begin(), nick.end(), nick.begin(), ::tolower);
    if(doesNicknameExists(nick)) return false;
    mRemoteUsers[nick] = user;
    return true;
}

bool Mainframe::changeLocalNickname(std::string old, std::string recent) {
	std::transform(recent.begin(), recent.end(), recent.begin(), ::tolower);
	std::transform(old.begin(), old.end(), old.begin(), ::tolower);
    LocalUser* tmp = mLocalUsers[old];
    mLocalUsers.erase(old);
    mLocalUsers[recent] = tmp;
    return true;
}

bool Mainframe::changeRemoteNickname(std::string old, std::string recent) {
	std::transform(recent.begin(), recent.end(), recent.begin(), ::tolower);
	std::transform(old.begin(), old.end(), old.begin(), ::tolower);
    RemoteUser* tmp = mRemoteUsers[old];
    mRemoteUsers.erase(old);
    mRemoteUsers[recent] = tmp;
    return true;
}

void Mainframe::removeLocalUser(std::string nick) {
	if(!doesNicknameExists(nick)) return;
	std::transform(nick.begin(), nick.end(), nick.begin(), ::tolower);
	auto it = mLocalUsers.find(nick);
	it->second = nullptr;
	delete it->second;
	mLocalUsers.erase (nick);
}

void Mainframe::removeRemoteUser(std::string nick) {
	if(!doesNicknameExists(nick)) return;
	std::transform(nick.begin(), nick.end(), nick.begin(), ::tolower);
	auto it = mRemoteUsers.find(nick);
	it->second = nullptr;
	delete it->second;
	mRemoteUsers.erase (nick);
}

LocalUser* Mainframe::getLocalUserByName(std::string nick) {
	std::transform(nick.begin(), nick.end(), nick.begin(), ::tolower);
    if(! doesNicknameExists(nick) ) return nullptr;
    return mLocalUsers[nick];
}

RemoteUser* Mainframe::getRemoteUserByName(std::string nick) {
	std::transform(nick.begin(), nick.end(), nick.begin(), ::tolower);
    if(! doesNicknameExists(nick) ) return nullptr;
    return mRemoteUsers[nick];
}

User *Mainframe::getUserByName(std::string nick) {
	std::transform(nick.begin(), nick.end(), nick.begin(), ::tolower);
    if(! doesNicknameExists(nick) ) return nullptr;
    else if (mLocalUsers.find(nick) != mLocalUsers.end()) return dynamic_cast<User*>(mLocalUsers[nick]);
    else if (mRemoteUsers.find(nick) != mRemoteUsers.end()) return dynamic_cast<User*>(mRemoteUsers[nick]);
    else return nullptr;
}

bool Mainframe::doesChannelExists(std::string name) {
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    return (mChannels.find(name) != mChannels.end());
}

void Mainframe::addChannel(Channel* chan) {
	std::string channame = chan->name();
	std::transform(channame.begin(), channame.end(), channame.begin(), ::tolower);
    if(!doesChannelExists(channame)) {
        mChannels[channame] = chan;
    }
}

void Mainframe::removeChannel(std::string name) {
	if(!doesChannelExists(name)) return;
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);
	auto it = mChannels.find(name);
	it->second = nullptr;
	delete it->second;
	mChannels.erase (name);
}

Channel* Mainframe::getChannelByName(std::string name) {
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    if(!doesChannelExists(name))    return nullptr;
    return mChannels[name];
}

void Mainframe::removeAllChannels() {
    auto it = mChannels.begin();
    for(; it != mChannels.end(); ++it) {
        delete (it->second);
    }
}

void Mainframe::removeAllUsers() {
    auto it = mLocalUsers.begin();
    for(; it != mLocalUsers.end(); ++it) {
        delete (it->second);
    }
    auto it2 = mRemoteUsers.begin();
    for(; it2 != mRemoteUsers.end(); ++it) {
        delete (it2->second);
    }
}

std::map<std::string, Channel*> Mainframe::channels() const { return mChannels; }

std::map<std::string, LocalUser*> Mainframe::LocalUsers() const { return mLocalUsers; }

std::map<std::string, RemoteUser*> Mainframe::RemoteUsers() const { return mRemoteUsers; }

int Mainframe::countchannels() { return mChannels.size(); }

int Mainframe::countusers() { return (mLocalUsers.size() + mRemoteUsers.size()); }
