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
#pragma once

#include <map>
#include <string>

#include "Channel.h"

class Mainframe {
  public:
	static Mainframe*   instance();
	~Mainframe();
	bool doesNicknameExists(std::string nick);
	bool addLocalUser(LocalUser* user, std::string nick);
	bool addRemoteUser(RemoteUser* user, std::string nick);
	void removeLocalUser(std::string nick);
	void removeRemoteUser(std::string nick);
	bool changeLocalNickname(std::string old, std::string recent);
	bool changeRemoteNickname(std::string old, std::string recent);
	LocalUser* getLocalUserByName(std::string nick);
	RemoteUser* getRemoteUserByName(std::string nick);
	User* getUserByName(std::string nick);
	bool doesChannelExists(std::string name);
	void addChannel(Channel* chan);
	void removeChannel(std::string name);

	Channel* getChannelByName(std::string name);

	std::map<std::string, Channel*> channels() const;
	std::map<std::string, LocalUser*> LocalUsers() const;
	std::map<std::string, RemoteUser*> RemoteUsers() const;

	int countchannels();
	int countusers();

  private:

	Mainframe() = default;
	Mainframe(const Mainframe&);
	Mainframe& operator=(Mainframe&) { return *this; };

	void removeAllChannels();
	void removeAllUsers();

	static Mainframe* mInstance;
	std::map<std::string, LocalUser*> mLocalUsers;
	std::map<std::string, RemoteUser*> mRemoteUsers;
	std::map<std::string, Channel*> mChannels;
};

