#ifndef CHANNEL_H
#define CHANNEL_H

#include <set>
#include <string>
#include "user.h"
#include "config.h"

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/bind.hpp>

typedef std::set<User*> UserSet;

extern boost::asio::io_context channel_user_context;

class Ban
{
	private:
		std::string canal;
		std::string mascara;
		std::string who;
		time_t fecha;
	public:
		Ban (std::string &channel, std::string &mask, std::string &whois, time_t tim) : canal(channel), mascara(mask), who(whois), fecha(tim), deadline(channel_user_context) {};
		~Ban () { deadline.cancel(); };
		std::string mask();
		std::string whois();
		time_t 		time();
		boost::asio::deadline_timer deadline;
		void check_expire(std::string canal, const boost::system::error_code &e);
		void expire(std::string canal);
};

typedef std::set<Ban*> BanSet;

class Channel {
    
public:

        Channel(User* creator, const std::string& name, const std::string& topic = "");
        ~Channel() { deadline.cancel(); };

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
		void check_flood(const boost::system::error_code &e);
		void broadcast_join(User* user, bool toUser);
		
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

    private:

        const std::string mName;
        std::string mTopic;
        UserSet mUsers;
        UserSet mOperators;
        UserSet mHalfOperators;
        UserSet mVoices;
        BanSet mBans;
		int flood;
		bool is_flood;
        bool mode_r;
        time_t lastflood;
        boost::asio::deadline_timer deadline;
};

#endif
