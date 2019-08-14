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

#include <set>
#include <string>
#include "ZeusBaseClass.h"
#include "Config.h"
#include "Timer.h"

typedef std::set<LocalUser*> LocalUserSet;
typedef std::set<RemoteUser*> RemoteUserSet;
extern TimerWheel timers;


class Ban
{
	public:
		Ban (std::string &channel, std::string &mask, std::string &whois, time_t tim) : canal(channel), mascara(mask), who(whois), fecha(tim), banexpire(this) {};
		~Ban () { };
		std::string mask();
		std::string whois();
		time_t 		time();
		void ExpireBan();
		void ExpireBanTimers(TimerWheel* timers);

		std::string canal;
		std::string mascara;
		std::string who;
		time_t fecha;
		MemberTimerEvent<Ban, &Ban::ExpireBan> banexpire;
};

typedef std::set<Ban*> BanSet;

class Channel {
    
public:

        Channel(LocalUser* creator, const std::string name);
        Channel(RemoteUser* creator, const std::string name);
        ~Channel() { };

		static Channel *FindChannel(std::string name);
		static void addChannel(Channel* chan);
		static void removeChannel(std::string name);

        void addUser(LocalUser* user);
        void removeUser(LocalUser* user);
        bool hasUser(LocalUser* user);
        bool isOperator(LocalUser* user);
        void delOperator(LocalUser* user);
        void giveOperator(LocalUser* user);
        bool isHalfOperator(LocalUser* user);
        void delHalfOperator(LocalUser* user);
        void giveHalfOperator(LocalUser* user);
        bool isVoice(LocalUser* user);
        void delVoice(LocalUser* user);
        void giveVoice(LocalUser* user);
        
        void addUser(RemoteUser* user);
        void removeUser(RemoteUser* user);
        bool hasUser(RemoteUser* user);
        bool isOperator(RemoteUser* user);
        void delOperator(RemoteUser* user);
        void giveOperator(RemoteUser* user);
        bool isHalfOperator(RemoteUser* user);
        void delHalfOperator(RemoteUser* user);
        void giveHalfOperator(RemoteUser* user);
        bool isVoice(RemoteUser* user);
        void delVoice(RemoteUser* user);
        void giveVoice(RemoteUser* user);

        void sendUserList(LocalUser* user);
        void sendWhoList(LocalUser* user);
        void broadcast(const std::string message);
        void broadcast_except_me(const std::string nick, const std::string message);
		void broadcast_join(LocalUser* user, bool toUser);
		void broadcast_away(LocalUser *user, std::string away, bool on);
		
        std::string password() const;
        std::string name() const;
        std::string topic() const; 
        unsigned int userCount() const;
        bool empty() const;
        bool full() const;
        bool hasPass() const;
        bool limited() const;
        bool getMode(char mode);
        void setMode(char mode, bool option);
        BanSet bans();
        void UnBan(Ban *ban);
        void setBan(std::string mask, std::string whois);
        void SBAN(std::string mask, std::string whois, std::string time);
        bool IsBan(std::string mask);
        void resetflood();
        void increaseflood();
        bool isonflood();
        void ExpireFloodTimers(TimerWheel* timers);
        void ExpireFlood();

        const std::string mName;
        std::string mTopic;
        LocalUserSet mLocalUsers;
        LocalUserSet mLocalOperators;
        LocalUserSet mLocalHalfOperators;
        LocalUserSet mLocalVoices;
        RemoteUserSet mRemoteUsers;
        RemoteUserSet mRemoteOperators;
        RemoteUserSet mRemoteHalfOperators;
        RemoteUserSet mRemoteVoices;
        BanSet mBans;
		int flood;
		bool is_flood;
        bool mode_r;
        time_t lastflood;
        std::mutex mtx;
        MemberTimerEvent<Channel, &Channel::ExpireFlood> expireflood;
};
