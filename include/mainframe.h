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

#include <boost/asio.hpp>

class User;
class Channel;

typedef std::map<std::string, User*>    UserMap; 
typedef std::map<std::string, Channel*> ChannelMap;

class Mainframe : public std::enable_shared_from_this<Mainframe> {

public:

        static Mainframe*   instance();
		void		start(std::string ip, int port, bool ssl, bool ipv6);
		void		server(std::string ip, int port, bool ssl, bool ipv6);
		void		ws(std::string ip, int port, bool ssl, bool ipv6);
		static void	timer();
		static void	queue();
		
		bool    doesNicknameExists(std::string nick);
        bool    addUser(User* user, std::string nick);
		void    removeUser(std::string nick);
		bool    changeNickname(std::string old, std::string recent);
        User*   getUserByName(std::string nick);

        bool    doesChannelExists(std::string name);
        void    addChannel(Channel* chan);
        void    removeChannel(std::string name);

        Channel* getChannelByName(std::string name);
        ChannelMap channels() const;
        UserMap users() const;
        int countchannels();
		int countusers();
        
    private:

		Mainframe() = default;
        ~Mainframe(); 
        Mainframe(const Mainframe&);
		Mainframe& operator=(Mainframe&) { return *this; };

        void removeAllChannels();
        void removeAllUsers();

        static Mainframe* mInstance; 
        UserMap mUsers;
        ChannelMap mChannels;
};

