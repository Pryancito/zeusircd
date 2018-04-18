#include "user.h"
#include "mainframe.h"
#include "oper.h"
#include "sha256.h"
#include "services.h"
#include "session.h"
#include "ircv3.h"
#include "parser.h"

extern time_t encendido;
extern CloneMap mClones;
extern OperSet miRCOps;

User::User(Session*     mysession, std::string server)
:   mSession(mysession), mServer(server), bSentUser(false), bSentNick(false), bSentMotd(false), bProperlyQuit(false),
	mode_r(false), mode_z(false), mode_o(false), mode_w(false) { bLogin = bPing = time(0); mIRCv3 = new Ircv3(this); }

User::~User() {
    if(!bProperlyQuit) {
		Parser::log("nick " + this->nick() + " sale del chat");
        ChannelSet::iterator it = mChannels.begin();
        for(; it != mChannels.end(); ++it) {
            (*it)->broadcast(messageHeader() + "QUIT :QUIT" + config->EOFMessage);
            (*it)->removeUser(this);
        }
		mClones[mHost] -=1;
		if (this->getMode('o') == true)
			miRCOps.erase(this);
		if (this->server() == config->Getvalue("serverName"))
			Servidor::sendall("QUIT " + mNickName);
		Mainframe::instance()->removeUser(mNickName);
    }
}

void User::cmdNick(const std::string& newnick) {
    if(bSentNick) {
        if(Mainframe::instance()->changeNickname(mNickName, newnick)) {
            mSession->sendAsUser("NICK :"+ newnick + config->EOFMessage);
            Parser::log("Nick " + mNickName + " se cambia el nick a: " + newnick + " con ip: " + mHost);
            ChannelSet::iterator it = mChannels.begin();
            for(; it != mChannels.end(); ++it)
                (*it)->broadcast_except_me(this, messageHeader() + "NICK " + newnick + config->EOFMessage);
            setNick(newnick);
            NickServ::checkmemos(this);
            
        } else {
            mSession->sendAsServer(ToString(Response::Error::ERR_NICKCOLLISION) + " " 
				+ mNickName + " " 
				+ newnick + " :El nick ya esta en uso." 
				+ config->EOFMessage);
        }
    } else {
		if (Mainframe::instance()->addUser(this, newnick)) {
			setNick(newnick);
			if (this->getMode('w') == false) {
				mHost = mSession->ip();
				mCloak = sha256(mSession->ip()).substr(0, 16);
			}
			Parser::log("Nick " + newnick + " entra al irc con ip: " + mHost);
			mSession->send(":" + newnick + " NICK :"+ newnick + config->EOFMessage);
			bSentNick = true;

			if(bSentNick && !bSentMotd) {
				bSentMotd = true;
				
				struct tm *tm = localtime(&encendido);
				char date[30];
				strftime(date, sizeof(date), "%r %d-%m-%Y", tm);
				std::string fecha = date;
				mSession->sendAsServer("001 " + mNickName + " :Bienvenido a \002" + config->Getvalue("network") + "\002" + config->EOFMessage);
				mSession->sendAsServer("002 " + mNickName + " :Tu servidor es: " + config->Getvalue("serverName") + " funcionando con: " + config->version + config->EOFMessage);
				mSession->sendAsServer("003 " + mNickName + " :Este servidor fue creado: " + fecha + config->EOFMessage);
				mSession->sendAsServer("004 " + mNickName + " " + config->Getvalue("serverName") + " " + config->version + " rzoiws robtkmlvshn r" + config->EOFMessage);
				mSession->sendAsServer("005 " + mNickName + " NETWORK=" + config->Getvalue("network") + " are supported by this server" + config->EOFMessage);
				mSession->sendAsServer("005 " + mNickName + " NICKLEN=" + config->Getvalue("nicklen") + " MAXCHANNELS=" + config->Getvalue("maxchannels") + " CHANNELLEN=" + config->Getvalue("chanlen") + " are supported by this server" + config->EOFMessage);
				mSession->sendAsServer("005 " + mNickName + " PREFIX=(ohv)@%+ STATUSMSG=@%+ are supported by this server" + config->EOFMessage);
				mSession->sendAsServer("002 " + mNickName + " :Hay \002" + std::to_string(Mainframe::instance()->countusers()) + "\002 usuarios y \002" + std::to_string(Mainframe::instance()->countchannels()) + "\002 canales." + config->EOFMessage);
				mSession->sendAsServer("002 " + mNickName + " :Hay \002" + std::to_string(NickServ::GetNicks()) + "\002 nicks registrados y \002" + std::to_string(ChanServ::GetChans()) + "\002 canales registrados." + config->EOFMessage);
				mSession->sendAsServer("002 " + mNickName + " :Hay \002" + std::to_string(Oper::Count()) + "\002 iRCops conectados." + config->EOFMessage);
				mSession->sendAsServer("002 " + mNickName + " :Hay \002" + std::to_string(Servidor::count()) + "\002 servidores conectados." + config->EOFMessage);
				if (mSession->ssl == true) {
					this->setMode('z', true);
					mSession->sendAsServer("MODE " + this->nick() + " +z" + config->EOFMessage);
				}
				std::string modos = "+";
				if (this->getMode('r') == true)
					modos.append("r");
				if (this->getMode('z') == true)
					modos.append("z");
				if (this->getMode('w') == true)
					modos.append("w");
				if (this->getMode('o') == true)
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
				+ newnick + " :El nick ya esta en uso." 
				+ config->EOFMessage);
		}
    }
}

void User::cmdUser(const std::string& ident) {
    if(bSentUser) {
        mSession->sendAsServer(ToString(Response::Error::ERR_ALREADYREGISTRED) + " " 
			+ mNickName 
			+ " You are already registered !" 
			+ config->EOFMessage);
    } else {
        mIdent = ident;
        bSentUser = true;
        if (!mNickName.empty())
			Servidor::sendall("SUSER " + mNickName + " " + ident);
    }
}

void User::cmdWebIRC(const std::string& ip) {
	mClones[mHost] -= 1;
	mCloak = sha256(ip).substr(0, 16);
	mHost = ip;
	mClones[mHost] += 1;
	this->setMode('w', true);
	mSession->sendAsServer("MODE " + mNickName + " +w" + config->EOFMessage);
	Servidor::sendall("UMODE " + mNickName + " +w");
	Servidor::sendall("WEBIRC " + mNickName + " " + ip);
}

void User::cmdQuit() {
	Parser::log("nick " + this->nick() + " sale del chat");
    ChannelSet::iterator it = mChannels.begin();
    for(; it != mChannels.end(); ++it) {
		(*it)->broadcast(messageHeader() + "QUIT :QUIT" + config->EOFMessage);
		(*it)->removeUser(this);
		if ((*it)->userCount() == 0)
			Mainframe::instance()->removeChannel((*it)->name());
    }
	mClones[mHost] -=1;
	if (this->getMode('o') == true)
		miRCOps.erase(this);
	if (this->server() == config->Getvalue("serverName"))
		Servidor::sendall("QUIT " + mNickName);
    Mainframe::instance()->removeUser(mNickName);
    mSession->close();
    bProperlyQuit = true;
}

void User::cmdPart(Channel* channel) {
	Parser::log("nick " + this->nick() + " sale del canal: " + channel->name());
    channel->broadcast(messageHeader() + "PART " + channel->name() + config->EOFMessage);
    channel->removeUser(this);
    mChannels.erase(channel);
}

void User::cmdJoin(Channel* channel) {
    mChannels.insert(channel);
    channel->addUser(this);
    Parser::log("nick " + this->nick() + " entra al canal: " + channel->name());
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

void User::cmdPing(std::string response) {
    mSession->sendAsServer("PONG " + config->Getvalue("serverName") + " :" + response + config->EOFMessage);
    bPing = time(0);
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

Ircv3* 		User::iRCv3() const {
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

void User::setHost(const std::string& host) {
    mHost = host;
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
	ChannelSet::iterator it = mChannels.begin();
	for(; it != mChannels.end(); ++it) {
		(*it)->broadcast_except_me(this, messageHeader() + "PART " + (*it)->name() + config->EOFMessage);
		Servidor::sendall("SPART " + this->nick() + " " + (*it)->name());
		if (this->iRCv3()->HasCapab("extended-join") == true && this->getMode('r') == true)
			(*it)->broadcast_except_me(this, messageHeader() + "JOIN " + (*it)->name() + " " + mNickName + " :ZeusiRCd" + config->EOFMessage);
		else
			(*it)->broadcast_except_me(this, messageHeader() + "JOIN :" + (*it)->name() + config->EOFMessage);
		Servidor::sendall("SJOIN " + this->nick() + " " + (*it)->name() + " +x");
	}
}

void User::propagatenick(std::string nickname) {
	ChannelSet::iterator it = mChannels.begin();
	for(; it != mChannels.end(); ++it) {
		(*it)->broadcast(messageHeader() + "NICK " + nickname + config->EOFMessage);
	}
}

void User::SNICK(std::string nickname, std::string ident, std::string host, std::string cloak, std::string login, std::string modos) {
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
	Parser::log("nick " + this->nick() + " entra al canal: " + channel->name());
    mChannels.insert(channel);
    channel->addUser(this);
    if (this->iRCv3()->HasCapab("extended-join") == true && this->getMode('r') == true)
		channel->broadcast(messageHeader() + "JOIN " + channel->name() + " " + mNickName + " :ZeusiRCd" + config->EOFMessage);
	else
		channel->broadcast(messageHeader() + "JOIN :" + channel->name() + config->EOFMessage);
}

void User::SKICK(Channel* channel) {
    channel->removeUser(this);
    mChannels.erase(channel);
}

void User::QUIT() {
	Parser::log("nick " + this->nick() + " sale del chat");
    ChannelSet::iterator it = mChannels.begin();
    for(; it != mChannels.end(); ++it) {
		(*it)->broadcast(messageHeader() + "QUIT :QUIT" + config->EOFMessage);
		(*it)->removeUser(this);
		if ((*it)->userCount() == 0)
			Mainframe::instance()->removeChannel((*it)->name());
    }
	mClones[mHost] -=1;
	if (this->getMode('o') == true)
		miRCOps.erase(this);
    Mainframe::instance()->removeUser(mNickName);
    bProperlyQuit = true;
}

void User::NETSPLIT() {
	Parser::log("nick " + this->nick() + " sale del chat por un netsplit");
    ChannelSet::iterator it = mChannels.begin();
    for(; it != mChannels.end(); ++it) {
		(*it)->broadcast(messageHeader() + "QUIT :*.net.split" + config->EOFMessage);
		(*it)->removeUser(this);
		if ((*it)->userCount() == 0)
			Mainframe::instance()->removeChannel((*it)->name());
    }
	mClones[mHost] -=1;
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
	ChannelSet::iterator it = mChannels.begin();
	for(; it != mChannels.end(); ++it) {
		if (ChanServ::HasMode((*it)->name(), "NONICKCHANGE") == true && (*it)->isonflood() == true && this->getMode('o') == false)
			return false;
	}
	return true;
}

void User::WEBIRC(const std::string& ip) {
	mClones[mHost] -= 1;
	mCloak = sha256(ip).substr(0, 16);
	mHost = ip;
	mClones[mHost] += 1;
}
