#pragma once

#include <boost/algorithm/string.hpp>
#include <string>
#include <vector>
#include <set>

class Session;
class Channel;
class Ircv3;

typedef std::vector<std::string> StrVec;
typedef std::set<Channel*> ChannelSet;

class User {
    friend class Session;
    friend class Ircv3;

    public:

        User(Session*   mysession, std::string server);
        ~User();
        void cmdNick(const std::string& newnick);
        void cmdUser(const std::string& ident);
		void cmdQuit();
        void cmdJoin(Channel* channel);
        void cmdPart(Channel* channel);
        void cmdKick(User* victim, const std::string& reason, Channel* channel);
        void cmdPing(std::string response);
        void cmdWebIRC(const std::string& ip);
		void UpdatePing();
		time_t GetPing();
		time_t GetLogin();

        Session* session() const;
        Ircv3* iRCv3() const;
        std::string nick() const;
        std::string ident() const;
        std::string host() const;
        std::string cloak() const;
        std::string sha() const;
        std::string server() const;
        std::string messageHeader() const;
        
        void setNick(const std::string& nick);
        void setHost(const std::string& host);
        
        void setMode(char mode, bool option);
        bool getMode(char mode);
        void Cycle();
        void SNICK(std::string nickname, std::string ident, std::string host, std::string cloak, std::string login, std::string modos);
        void SUSER(const std::string& ident);
        void SJOIN(Channel* channel);
        void SKICK(Channel* channel);
        void QUIT();
        void NETSPLIT();
        void WEBIRC(const std::string& ip);
        void propagatenick(std::string nickname);
        int Channels();
        bool canchangenick();
		
private:

		Session* mSession;
		Ircv3 *mIRCv3;
		
        std::string mIdent;
        std::string mNickName;
        std::string mHost;
		std::string mCloak;
		std::string mServer;
        
        bool bSentUser;
        bool bSentNick;
        bool bSentMotd;
        bool bProperlyQuit;
		time_t bPing;
		time_t bLogin;

        bool mode_r;
        bool mode_z;
        bool mode_o;
        bool mode_w;

        ChannelSet mChannels;
};
