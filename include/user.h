#ifndef USER_H
#define USER_H

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/bind.hpp>

#include <string>
#include <vector>
#include <set>

#define GC_THREADS
#define GC_ALWAYS_MULTITHREADED
#include <gc_cpp.h>
#include <gc.h>

class Session;
class Channel;
class Ircv3;

typedef std::vector<std::string> StrVec;
typedef std::set<Channel*> ChannelSet;

class User : public gc_cleanup {
    friend class Session;
    friend class Ircv3;

    public:

        User(Session*   mysession, const std::string &server);
        ~User();
        void cmdNick(const std::string& newnick);
        void cmdUser(const std::string& ident);
		void cmdQuit();
        void cmdJoin(Channel* channel);
        void cmdPart(Channel* channel);
        void cmdKick(User* victim, const std::string& reason, Channel* channel);
        void cmdPing(const std::string &response);
        void cmdWebIRC(const std::string& ip);
		void UpdatePing();
		void setPass(const std::string& password);
		bool ispassword();
		time_t GetPing();
		time_t GetLogin();

        Session* session() const;
        Ircv3 *iRCv3() const;
        std::string nick() const;
        std::string ident() const;
        std::string host() const;
        std::string cloak() const;
        std::string sha() const;
        std::string server() const;
        std::string messageHeader() const;
        std::string getPass();
        bool connclose();
        
        void setNick(const std::string& nick);
        void setHost(const std::string& host);
        
        void setMode(char mode, bool option);
        bool getMode(char mode);
        void Cycle();
        void SNICK(const std::string &nickname, const std::string &ident, const std::string &host, const std::string &cloak, std::string login, std::string modos);
        void SUSER(const std::string& ident);
        void SJOIN(Channel* channel);
        void SKICK(Channel* channel);
        void QUIT();
        void NETSPLIT();
        void WEBIRC(const std::string& ip);
        void NICK(const std::string& nickname);
        void propagatenick(const std::string &nickname);
        int Channels();
        bool canchangenick();
		void check_ping(const boost::system::error_code &e);
		
private:

		Session* mSession;
		Ircv3 *mIRCv3;
		
        std::string mIdent;
        std::string mNickName;
        std::string mHost;
		std::string mCloak;
		std::string mServer;
		std::string PassWord;
        
        bool bSentUser;
        bool bSentNick;
        bool bSentMotd;
        bool bProperlyQuit;
        bool bSentPass;
		time_t bPing;
		time_t bLogin;

        bool mode_r;
        bool mode_z;
        bool mode_o;
        bool mode_w;

		boost::asio::deadline_timer deadline;

        ChannelSet mChannels;
};

#endif
