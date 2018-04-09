#pragma once

#include <set>
#include <string>
#include "user.h"

typedef std::set<User*> UserSet;

class Ban
{
	private:
		std::string mascara;
		std::string who;
		time_t fecha;
	public:
		Ban (std::string mask, std::string whois, time_t tim) : mascara(mask), who(whois), fecha(tim) { };
		~Ban () {};
		std::string mask();
		std::string whois();
		time_t 		time();
};

typedef std::set<Ban*> BanSet;

class Channel {
    
public:

        Channel(User* creator, const std::string& name, const std::string& topic = "");
        ~Channel();

        void cmdOPlus(User *user, User *victim);
        void cmdOMinus(User *user);
        void cmdKPlus(const std::string& newPass);
        void cmdKMinus();
        void cmdLPlus(const std::string& newNbPlace);
        void cmdLMinus();
        void cmdTopic(const std::string& topic);

        void addUser(User* user);
        void removeUser(User* user);
        bool hasUser(User* user);
        bool isOperator(User* user);
        void delOperator(User* user);
        void giveOperator(User* user);
        bool isHalfOperator(User* user);
        void delHalfOperator(User* user);
        void giveHalfOperator(User* user);
        bool isVoice(User* user);
        void delVoice(User* user);
        void giveVoice(User* user);
        void sendUserList(User* user);
        void sendWhoList(User* user);
        void broadcast(const std::string& message);
        void broadcast_except_me(User* user, const std::string& message);

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
        UserSet users();
        void UnBan(Ban *ban);
        void setBan(std::string mask, std::string whois);
        void SBAN(std::string mask, std::string whois, std::string time);
        bool IsBan(std::string mask);
        void resetflood();
        void increaseflood();
        bool isonflood();
        void setid(std::string ide);
        std::string getid();

    private:

        const std::string mName;
        std::string mTopic;
        std::string mID;
        UserSet mUsers;
        UserSet mOperators;
        UserSet mHalfOperators;
        UserSet mVoices;
        BanSet mBans;
		int flood;
        bool mode_r;
};
