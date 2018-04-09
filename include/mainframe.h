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
		
		bool    doesNicknameExists(const std::string& nick);
        bool    addUser(User* user, std::string nick);
        bool	addUser(User* user, std::string nick, std::string ide);
		void    removeUser(const std::string& nick);
		bool    changeNickname(const std::string& old, const std::string& recent);
        User*   getUserByName(const std::string& nick);
        User*	getUserByID(const std::string& id);

        bool    doesChannelExists(const std::string& name);
        void    addChannel(Channel* chan, std::string channame);
        void	addChannel(Channel* chan, std::string channame, std::string ide);
        void    removeChannel(const std::string& name);

        Channel* getChannelByName(const std::string& name);
        Channel* getChannelByID(const std::string& id);
        //void    updateChannels();
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

        static Mainframe* mInstance; 
        UserMap mUsers;
        ChannelMap mChannels;
};
