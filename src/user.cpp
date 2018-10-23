#include "user.h"
#include "mainframe.h"
#include "oper.h"
#include "sha256.h"
#include "services.h"
#include "session.h"
#include "ircv3.h"
#include "parser.h"
#include "utils.h"

extern time_t encendido;
extern OperSet miRCOps;
extern boost::asio::io_context channel_user_context;

User::User(Session*     mysession, const std::string &server)
:   mSession(mysession), mServer(server), bSentUser(false), bSentNick(false), bSentMotd(false), bProperlyQuit(false), bSentPass(false), bPing(0), bLogin(0),
	mode_r(false), mode_z(false), mode_o(false), mode_w(false), deadline(channel_user_context) {
		mIRCv3 = new Ircv3(this);
	}

User::~User() {
    if(!bProperlyQuit && bSentNick) {
		Parser::log("El nick " + this->nick() + " sale del chat");
        ChannelSet::iterator it = mChannels.begin();
        for(; it != mChannels.end(); ++it) {
            (*it)->broadcast(messageHeader() + "QUIT :QUIT" + config->EOFMessage);
            (*it)->removeUser(this);
            if ((*it)->userCount() == 0)
				Mainframe::instance()->removeChannel((*it)->name());
        }
		if (this->getMode('o') == true)
			miRCOps.erase(this);
		if (this->server() == config->Getvalue("serverName")) {
			Servidor::sendall("QUIT " + mNickName);
			mSession->close();
			deadline.cancel();
		}
		delete mIRCv3;
		Mainframe::instance()->removeUser(mNickName);
    }
}

void User::cmdNick(const std::string& newnick) {
    if(bSentNick) {
        if(Mainframe::instance()->changeNickname(mNickName, newnick)) {
            mSession->sendAsUser("NICK :"+ newnick + config->EOFMessage);
            Parser::log(Utils::make_string("", "Nickname %s changes nick to: %s with ip: %s", mNickName.c_str(), newnick.c_str(), mHost.c_str()));
			Servidor::sendall("NICK " + mNickName + " " + newnick);
            ChannelSet::iterator it = mChannels.begin();
            for(; it != mChannels.end(); ++it) {
                (*it)->broadcast_except_me(this, messageHeader() + "NICK " + newnick + config->EOFMessage);
                ChanServ::CheckModes(this, (*it)->name());
            }
            setNick(newnick);
            NickServ::checkmemos(this);
            if (OperServ::IsOper(newnick) == true && getMode('o') == false) {
				miRCOps.insert(this);
				setMode('o', true);
				mSession->sendAsServer("MODE " + nick() + " +o" + config->EOFMessage);
				Servidor::sendall("UMODE " + nick() + " +o");
			} else if (getMode('o') == true && OperServ::IsOper(newnick) == false) {
				miRCOps.erase(this);
				setMode('o', false);
				mSession->sendAsServer("MODE " + this->nick() + " -o" + config->EOFMessage);
				Servidor::sendall("UMODE " + nick() + " -o");
			}
            if (NickServ::GetvHost(newnick) != "")
				Cycle();
        } else {
            mSession->sendAsServer(ToString(Response::Error::ERR_NICKCOLLISION) + " " 
				+ mNickName + " " 
				+ newnick + " :" + Utils::make_string(mNickName, "Nick is in use.") 
				+ config->EOFMessage);
        }
    } else {
		if (Mainframe::instance()->addUser(this, newnick)) {
			setNick(newnick);
			if (this->getMode('w') == false) {
				mHost = mSession->ip();
				mCloak = sha256(mSession->ip()).substr(0, 16);
			}
			Parser::log(Utils::make_string("", "Nickname %s enter to irc with ip: %s", newnick.c_str(), mHost.c_str()));
			mSession->send(":" + newnick + " NICK :"+ newnick + config->EOFMessage);
			bPing = time(0);
			bLogin = time(0);
			bSentNick = true;

			deadline.expires_from_now(boost::posix_time::seconds(60));
			deadline.async_wait(boost::bind(&User::check_ping, this, boost::asio::placeholders::error));

			if(bSentNick && !bSentMotd) {
				bSentMotd = true;
				
				struct tm *tm = localtime(&encendido);
				char date[30];
				strftime(date, sizeof(date), "%r %d-%m-%Y", tm);
				std::string fecha = date;
				mSession->sendAsServer("001 " + mNickName + " :" + Utils::make_string(mNickName, "Welcome to \002%s.\002", config->Getvalue("network").c_str()) + config->EOFMessage);
				mSession->sendAsServer("002 " + mNickName + " :" + Utils::make_string(mNickName, "Your server is: %s working with: %s", config->Getvalue("serverName").c_str(), config->version.c_str()) + config->EOFMessage);
				mSession->sendAsServer("003 " + mNickName + " :" + Utils::make_string(mNickName, "This server was created: %s", fecha.c_str()) + config->EOFMessage);
				mSession->sendAsServer("004 " + mNickName + " " + config->Getvalue("serverName") + " " + config->version + " rzoiws robtkmlvshn r" + config->EOFMessage);
				mSession->sendAsServer("005 " + mNickName + " NETWORK=" + config->Getvalue("network") + " are supported by this server" + config->EOFMessage);
				mSession->sendAsServer("005 " + mNickName + " NICKLEN=" + config->Getvalue("nicklen") + " MAXCHANNELS=" + config->Getvalue("maxchannels") + " CHANNELLEN=" + config->Getvalue("chanlen") + " are supported by this server" + config->EOFMessage);
				mSession->sendAsServer("005 " + mNickName + " PREFIX=(ohv)@%+ STATUSMSG=@%+ are supported by this server" + config->EOFMessage);
				mSession->sendAsServer("002 " + mNickName + " :" + Utils::make_string(mNickName, "There are \002%s\002 users and \002%s\002 channels.", std::to_string(Mainframe::instance()->countusers()).c_str(), std::to_string(Mainframe::instance()->countchannels()).c_str()) + config->EOFMessage);
				mSession->sendAsServer("002 " + mNickName + " :" + Utils::make_string(mNickName, "There are \002%s\002 registered nicks and \002%s\002 registered channels.", std::to_string(NickServ::GetNicks()).c_str(), std::to_string(ChanServ::GetChans()).c_str()) + config->EOFMessage);
				mSession->sendAsServer("002 " + mNickName + " :" + Utils::make_string(mNickName, "There are \002%s\002 connected iRCops.", std::to_string(Oper::Count()).c_str()) + config->EOFMessage);
				mSession->sendAsServer("002 " + mNickName + " :" + Utils::make_string(mNickName, "There are \002%s\002 connected servers.", std::to_string(Servidor::count()).c_str()) + config->EOFMessage);
				if (mSession->ssl == true) {
					setMode('z', true);
					mSession->sendAsServer("MODE " + nick() + " +z" + config->EOFMessage);
				} if (mSession->websocket == true) {
					setMode('w', true);
					mSession->sendAsServer("MODE " + nick() + " +w" + config->EOFMessage);
				} if (OperServ::IsOper(mNickName) == true) {
					miRCOps.insert(this);
					setMode('o', true);
					mSession->sendAsServer("MODE " + nick() + " +o" + config->EOFMessage);
				}
				std::string modos = "+";
				if (getMode('r') == true)
					modos.append("r");
				if (getMode('z') == true)
					modos.append("z");
				if (getMode('w') == true)
					modos.append("w");
				if (getMode('o') == true)
					modos.append("o");
				std::string identi = "ZeusiRCd";
				if (!mIdent.empty())
					identi = mIdent;
				Servidor::sendall("SNICK " + mNickName + " " + identi + " " + mHost + " " + mCloak + " " + std::to_string(bLogin) + " " + mServer + " " + modos);
				NickServ::checkmemos(this);
			}
		} else {
			mSession->sendAsServer(ToString(Response::Error::ERR_NICKCOLLISION) + " " 
				+ mNickName + " " 
				+ newnick + " :" + Utils::make_string(mNickName, "Nick is in use.")
				+ config->EOFMessage);
		}
    }
}

void User::cmdUser(const std::string& ident) {
    if(bSentUser) {
        mSession->sendAsServer(ToString(Response::Error::ERR_ALREADYREGISTRED) + " " 
			+ mNickName 
			+ " " + Utils::make_string(mNickName, "You are already registered !")
			+ config->EOFMessage);
    } else {
        mIdent = ident;
        bSentUser = true;
        if (!mNickName.empty())
			Servidor::sendall("SUSER " + mNickName + " " + ident);
    }
}

void User::cmdWebIRC(const std::string& ip) {
	mCloak = sha256(ip).substr(0, 16);
	mHost = ip;
	this->setMode('w', true);
	mSession->sendAsServer("MODE " + mNickName + " +w" + config->EOFMessage);
	Servidor::sendall("UMODE " + mNickName + " +w");
	Servidor::sendall("WEBIRC " + mNickName + " " + ip);
}

void User::setPass(const std::string& password) {
	PassWord = password;
}

bool User::ispassword() {
	return (!PassWord.empty());
}

std::string User::getPass() {
	return PassWord;
}

void User::cmdQuit() {
	Parser::log(Utils::make_string("", "Nick %s leaves irc", nick().c_str()));
    ChannelSet::iterator it = mChannels.begin();
    for(; it != mChannels.end(); ++it) {
		(*it)->broadcast(messageHeader() + "QUIT :QUIT" + config->EOFMessage);
		(*it)->removeUser(this);
		if ((*it)->userCount() == 0)
			Mainframe::instance()->removeChannel((*it)->name());
    }
	if (this->getMode('o') == true)
		miRCOps.erase(this);
	if (this->server() == config->Getvalue("serverName")) {
		Servidor::sendall("QUIT " + mNickName);
		mSession->close();
		deadline.cancel();
	}
	delete mIRCv3;
    Mainframe::instance()->removeUser(mNickName);
    bProperlyQuit = true;
}

void User::cmdPart(Channel* channel) {
	Parser::log(Utils::make_string("", "Nick %s leaves channel: %s", nick().c_str(), channel->name().c_str()));
    channel->broadcast(messageHeader() + "PART " + channel->name() + config->EOFMessage);
    channel->removeUser(this);
    mChannels.erase(channel);
}

void User::cmdJoin(Channel* channel) {
    mChannels.insert(channel);
    channel->addUser(this);
    Parser::log(Utils::make_string("", "Nick %s joins channel: %s", nick().c_str(), channel->name().c_str()));
    if (this->iRCv3()->HasCapab("extended-join") == true) {
		if (this->getMode('r') == true)
			channel->broadcast(messageHeader() + "JOIN " + channel->name() + " " + mNickName + " :ZeusiRCd" + config->EOFMessage);
		else
			channel->broadcast(messageHeader() + "JOIN " + channel->name() + " * :ZeusiRCd" + config->EOFMessage);
	} else
		channel->broadcast(messageHeader() + "JOIN :" + channel->name() + config->EOFMessage);
    channel->sendUserList(this);
}

void User::cmdKick(User* victim, const std::string& reason, Channel* channel) {
    channel->broadcast(":" + mNickName + " KICK " + channel->name() + " " + victim->nick() + " :" + reason + config->EOFMessage);
}

void User::cmdPing(const std::string &response) {
    mSession->sendAsServer("PONG " + config->Getvalue("serverName") + " :" + response + config->EOFMessage);
    bPing = time(0);
}

void User::propagateimg(const std::string &sender, const std::string &target, const std::string &media, const std::string &image) {
	if (server() == config->Getvalue("serverName")) {
		if (this->iRCv3()->HasCapab("image-base64") == true)
			mSession->async_send(":" + sender + " BASE64 " + target + " " + media + " " + image +  config->EOFMessage);
	} else
		Servidor::sendall("BASE64 " + sender + " " + target + " " + media + " " + image);
}

void User::UpdatePing() {
	bPing = time(0);
}

time_t User::GetPing() {
	return bPing;
}

time_t User::GetLogin() {
	return bLogin;
}

Session*    User::session() const {
    return mSession;
}

Ircv3 	*	User::iRCv3() const {
	return mIRCv3;
}

std::string User::nick() const {
    return mNickName;
}

std::string User::ident() const {
    return mIdent;
}

std::string User::host() const {
    return mHost;
}

std::string User::sha() const {
	return mCloak;
}

std::string User::server() const {
	return mServer;
}

std::string User::cloak() const {
	if (NickServ::GetvHost(mNickName) != "")
		return NickServ::GetvHost(mNickName);
	else
		return mCloak;
}

bool User::connclose() {
	return (!bSentNick);
}

bool User::getMode(char mode) {
	switch (mode) {
		case 'o': return mode_o;
		case 'r': return mode_r;
		case 'z': return mode_z;
		case 'w': return mode_w;
		default: return false;
	}
	return false;
}

void User::setNick(const std::string& nick) {
    mNickName = nick;
}

void User::setMode(char mode, bool option) {
	switch (mode) {
		case 'o': mode_o = option; break;
		case 'r': mode_r = option; break;
		case 'z': mode_z = option; break;
		case 'w': mode_w = option; break;
		default: break;
	}
	return;
}

void User::Cycle() {
	if (Channels() == 0)
		return;
	ChannelSet::iterator it = mChannels.begin();
	for(; it != mChannels.end(); ++it) {
		(*it)->broadcast_except_me(this, messageHeader() + "PART " + (*it)->name() + config->EOFMessage);
		Servidor::sendall("SPART " + this->nick() + " " + (*it)->name());
		std::string mode = "+";
		if ((*it)->isOperator(this) == true)
			mode.append("o");
		else if ((*it)->isHalfOperator(this) == true)
			mode.append("h");
		else if ((*it)->isVoice(this) == true)
			mode.append("v");
		else
			mode.append("x");
			
		if (this->iRCv3()->HasCapab("extended-join") == true) {
			if (this->getMode('r') == true)
				(*it)->broadcast_except_me(this, messageHeader() + "JOIN " + (*it)->name() + " " + mNickName + " :ZeusiRCd" + config->EOFMessage);
			else
				(*it)->broadcast_except_me(this, messageHeader() + "JOIN " + (*it)->name() + " * :ZeusiRCd" + config->EOFMessage);
		} else {
			(*it)->broadcast_except_me(this, messageHeader() + "JOIN :" + (*it)->name() + config->EOFMessage);
		}
		if (mode != "+x")
			(*it)->broadcast_except_me(this, ":" + config->Getvalue("chanserv") + " MODE " + (*it)->name() + " " + mode + " " + this->nick() + config->EOFMessage);
		Servidor::sendall("SJOIN " + this->nick() + " " + (*it)->name() + " " + mode);
	}
}

void User::propagatenick(const std::string &nickname) {
	ChannelSet::iterator it = mChannels.begin();
	for(; it != mChannels.end(); ++it) {
		(*it)->broadcast(messageHeader() + "NICK " + nickname + config->EOFMessage);
	}
}

void User::SNICK(const std::string &nickname, const std::string &ident, const std::string &host, const std::string &cloak, std::string login, std::string modos) {
	mNickName = nickname;
	mIdent = ident;
	mHost = host;
	mCloak = cloak;
	bLogin = (time_t ) stoi(login);
	for (unsigned int i = 1; i < modos.size(); i++) {
		switch(modos[i]) {
			case 'o': this->setMode('o', true); miRCOps.insert(this); continue;
			case 'w': this->setMode('w', true); continue;
			case 'z': this->setMode('z', true); continue;
			case 'r': this->setMode('r', true); continue;
			default: continue;
		}
	}
}

void User::SUSER(const std::string& ident) {
	mIdent = ident;
}

void User::SJOIN(Channel* channel) {
	Parser::log(Utils::make_string("", "Nick %s joins channel: %s", nick().c_str(), channel->name().c_str()));
    mChannels.insert(channel);
    channel->addUser(this);
    if (this->iRCv3()->HasCapab("extended-join") == true && this->getMode('r') == true)
		channel->broadcast(messageHeader() + "JOIN " + channel->name() + " " + mNickName + " :ZeusiRCd" + config->EOFMessage);
	else if (this->iRCv3()->HasCapab("extended-join") == true && this->getMode('r') == false)
		channel->broadcast(messageHeader() + "JOIN " + channel->name() + " * :ZeusiRCd" + config->EOFMessage);
	else
		channel->broadcast(messageHeader() + "JOIN :" + channel->name() + config->EOFMessage);
}

void User::SKICK(Channel* channel) {
    channel->removeUser(this);
    mChannels.erase(channel);
}

void User::QUIT() {
	Parser::log(Utils::make_string("", "Nick %s leaves irc", nick().c_str()));
    ChannelSet::iterator it = mChannels.begin();
    for(; it != mChannels.end(); ++it) {
		(*it)->broadcast(messageHeader() + "QUIT :QUIT" + config->EOFMessage);
		(*it)->removeUser(this);
		if ((*it)->userCount() == 0)
			Mainframe::instance()->removeChannel((*it)->name());
    }
	if (this->getMode('o') == true)
		miRCOps.erase(this);
	if (this->server() == config->Getvalue("serverName")) {
		mSession->close();
		deadline.cancel();
	}
	delete mIRCv3;
    Mainframe::instance()->removeUser(mNickName);
    bProperlyQuit = true;
}

void User::NETSPLIT() {
	Parser::log(Utils::make_string("", "Nick %s leaves irc caused by a netsplit", nick().c_str()));
    ChannelSet::iterator it = mChannels.begin();
    for(; it != mChannels.end(); ++it) {
		(*it)->broadcast(messageHeader() + "QUIT :*.net.split" + config->EOFMessage);
		(*it)->removeUser(this);
		if ((*it)->userCount() == 0)
			Mainframe::instance()->removeChannel((*it)->name());
    }
	if (this->getMode('o') == true)
		miRCOps.erase(this);
    Mainframe::instance()->removeUser(mNickName);
    bProperlyQuit = true;
}

std::string User::messageHeader() const {
	if (NickServ::GetvHost(mNickName) != "")
		return std::string(":"+mNickName+"!"+mIdent+"@"+NickServ::GetvHost(mNickName)+" ");
	else
		return std::string(":"+mNickName+"!"+mIdent+"@"+mCloak+" ");
}

int User::Channels() {
	return mChannels.size();
}

bool User::canchangenick() {
	if (Channels() == 0)
		return true;
	ChannelSet::iterator it = mChannels.begin();
	for(; it != mChannels.end(); ++it) {
		if (getMode('o') == true)
			return true;
		if (ChanServ::IsRegistered((*it)->name()) == true && ChanServ::HasMode((*it)->name(), "NONICKCHANGE") == 1)
			return false;
		if ((*it)->isonflood() == true && this->getMode('o') == false)
			return false;
	}
	return true;
}

void User::WEBIRC(const std::string& ip) {
	mCloak = sha256(ip).substr(0, 16);
	mHost = ip;
}

void User::check_ping(const boost::system::error_code &e) {
	if (!e) {
		if (GetPing() + 3600 < time(0) && session() != nullptr && getMode('w') == true)
			cmdQuit();
		else if (GetPing() + 180 < time(0) && session() != nullptr)
			cmdQuit();
		else if (session() != nullptr) {
			session()->send("PING :" + config->Getvalue("serverName") + config->EOFMessage);
			deadline.cancel();
			deadline.expires_from_now(boost::posix_time::seconds(60));
			deadline.async_wait(boost::bind(&User::check_ping, this, boost::asio::placeholders::error));
		}
	}
}
