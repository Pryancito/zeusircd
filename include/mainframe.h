#pragma once

#include <map>
#include <string>

#include "server.h"
#include "user.h"
#include "channel.h"

typedef std::map<std::string, User*>    UserMap; 
typedef std::map<std::string, Channel*> ChannelMap;

class Mainframe {

public:

        static Mainframe*   instance();
		void    start(std::string ip, int port, bool ssl, bool ipv6);
		void    server(std::string ip, int port, bool ssl, bool ipv6);
		void    ws(std::string ip, int port, bool ssl, bool ipv6);
		
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

