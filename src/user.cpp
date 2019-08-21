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
std::mutex quit_mtx;
boost::asio::ssl::context fakectx(boost::asio::ssl::context::sslv23);
boost::asio::io_context fake;

User::User(const boost::asio::executor& ex, boost::asio::ssl::context &ctx, std::string server)
:   Session(ex, ctx), mIRCv3(this), mServer(server), mLang("en"), bSentUser(false), bSentNick(false), bSentMotd(false), bProperlyQuit(false), bSentPass(false), bPing(0), bLogin(0), bAway(false), bSentQuit(false),
	mode_r(false), mode_z(false), mode_o(false), mode_w(false), deadline_u(channel_user_context) {
	}
User::User(std::string server)
:   Session(), mIRCv3(nullptr), mServer(server), mLang("en"), bSentUser(false), bSentNick(false), bSentMotd(false), bProperlyQuit(false), bSentPass(false), bPing(0), bLogin(0), bAway(false), bSentQuit(false),
	mode_r(false), mode_z(false), mode_o(false), mode_w(false), deadline_u(channel_user_context) {
	}
	
User *User::mUser() { return this; };

User::~User() {
    if(!bProperlyQuit && bSentNick) {
		Parser::log("El nick " + nick() + " sale del chat");
		if (!bSentQuit)
			Exit();
		if (getMode('o') == true)
			miRCOps.erase(this);
		if (LocalUser == true) {
			Servidor::sendall("QUIT " + mNickName);
			close();
		}
		Mainframe::instance()->removeUser(mNickName);
    }
}

void User::Exit() {
	bSentQuit = true;
	quit_mtx.lock();
	for (auto channel : mChannels) {
		channel->removeUser(this);
		channel->broadcast(messageHeader() + "QUIT :QUIT" + config->EOFMessage);
		if (channel->userCount() == 0)
			Mainframe::instance()->removeChannel(channel->name());
	}
	quit_mtx.unlock();
}

void User::cmdNick(const std::string& newnick) {
    if(bSentNick) {
        if(Mainframe::instance()->changeNickname(mNickName, newnick)) {
			Parser::log(Utils::make_string("", "Nickname %s changes nick to: %s with ip: %s", mNickName.c_str(), newnick.c_str(), mHost.c_str()));
            sendAsUser("NICK :"+ newnick + config->EOFMessage);
			Servidor::sendall("NICK " + mNickName + " " + newnick);
			std::string oldheader = messageHeader();
			std::string oldnick = mNickName;
			setNick(newnick);
            for (auto channel : mChannels) {
                channel->broadcast_except_me(nick(), oldheader + "NICK " + newnick + config->EOFMessage);
                ChanServ::CheckModes(this, channel->name());
            }
            if (getMode('r') == true)
				NickServ::checkmemos(this);
            if (OperServ::IsOper(newnick) == true && getMode('o') == false) {
				miRCOps.insert(this);
				setMode('o', true);
				sendAsServer("MODE " + nick() + " +o" + config->EOFMessage);
				Servidor::sendall("UMODE " + nick() + " +o");
			} else if (getMode('o') == true && OperServ::IsOper(newnick) == false) {
				miRCOps.erase(this);
				setMode('o', false);
				sendAsServer("MODE " + nick() + " -o" + config->EOFMessage);
				Servidor::sendall("UMODE " + nick() + " -o");
			}
            if (NickServ::GetvHost(oldnick) != NickServ::GetvHost(mNickName)) {
				Cycle();
			}
        } else {
            sendAsServer("436 " 
				+ mNickName + " " 
				+ newnick + " :" + Utils::make_string(mNickName, "Nick is in use.") 
				+ config->EOFMessage);
        }
    } else {
		if (Mainframe::instance()->addUser(this, newnick)) {
			setNick(newnick);
			if (getMode('w') == false) {
				mHost = ip();
				mCloak = sha256(ip()).substr(0, 16);
			}
			Parser::log(Utils::make_string("", "Nickname %s enter to irc with ip: %s", newnick.c_str(), mHost.c_str()));
			send(":" + newnick + " NICK :"+ newnick + config->EOFMessage);
			bPing = time(0);
			bLogin = time(0);
			bSentNick = true;

			deadline_u.expires_from_now(boost::posix_time::seconds(60));
			deadline_u.async_wait(boost::bind(&User::check_ping, this, boost::asio::placeholders::error));

			if(bSentNick && !bSentMotd) {
				bSentMotd = true;
				
				struct tm tm;
				localtime_r(&encendido, &tm);
				char date[32];
				strftime(date, sizeof(date), "%r %d-%m-%Y", &tm);
				std::string fecha = date;
				sendAsServer("001 " + mNickName + " :" + Utils::make_string(mNickName, "Welcome to \002%s.\002", config->Getvalue("network").c_str()) + config->EOFMessage);
				sendAsServer("002 " + mNickName + " :" + Utils::make_string(mNickName, "Your server is: %s working with: %s", config->Getvalue("serverName").c_str(), config->version.c_str()) + config->EOFMessage);
				sendAsServer("003 " + mNickName + " :" + Utils::make_string(mNickName, "This server was created: %s", fecha.c_str()) + config->EOFMessage);
				sendAsServer("004 " + mNickName + " " + config->Getvalue("serverName") + " " + config->version + " rzoiws robtkmlvshn r" + config->EOFMessage);
				sendAsServer("005 " + mNickName + " NETWORK=" + config->Getvalue("network") + " are supported by this server" + config->EOFMessage);
				sendAsServer("005 " + mNickName + " NICKLEN=" + config->Getvalue("nicklen") + " MAXCHANNELS=" + config->Getvalue("maxchannels") + " CHANNELLEN=" + config->Getvalue("chanlen") + " are supported by this server" + config->EOFMessage);
				sendAsServer("005 " + mNickName + " PREFIX=(ohv)@%+ STATUSMSG=@%+ are supported by this server" + config->EOFMessage);
				sendAsServer("002 " + mNickName + " :" + Utils::make_string(mNickName, "There are \002%s\002 users and \002%s\002 channels.", std::to_string(Mainframe::instance()->countusers()).c_str(), std::to_string(Mainframe::instance()->countchannels()).c_str()) + config->EOFMessage);
				sendAsServer("002 " + mNickName + " :" + Utils::make_string(mNickName, "There are \002%s\002 registered nicks and \002%s\002 registered channels.", std::to_string(NickServ::GetNicks()).c_str(), std::to_string(ChanServ::GetChans()).c_str()) + config->EOFMessage);
				sendAsServer("002 " + mNickName + " :" + Utils::make_string(mNickName, "There are \002%s\002 connected iRCops.", std::to_string(Oper::Count()).c_str()) + config->EOFMessage);
				sendAsServer("002 " + mNickName + " :" + Utils::make_string(mNickName, "There are \002%s\002 connected servers.", std::to_string(Servidor::count()).c_str()) + config->EOFMessage);
				sendAsServer("422 " + mNickName + " :No MOTD" + config->EOFMessage);
				if (ssl == true) {
					setMode('z', true);
					sendAsServer("MODE " + nick() + " +z" + config->EOFMessage);
				} if (websocket == true) {
					setMode('w', true);
					sendAsServer("MODE " + nick() + " +w" + config->EOFMessage);
				} if (OperServ::IsOper(mNickName) == true) {
					miRCOps.insert(this);
					setMode('o', true);
					sendAsServer("MODE " + nick() + " +o" + config->EOFMessage);
				}
				sendAsServer("396 " + mNickName + " " + cloak() + " :is now your hidden host" + config->EOFMessage);
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
				else
					mIdent = identi;
				Servidor::sendall("SNICK " + mNickName + " " + identi + " " + mHost + " " + mCloak + " " + std::to_string(bLogin) + " " + mServer + " " + modos);
				if (getMode('r') == true)
					NickServ::checkmemos(this);
			}
		} else {
			sendAsServer("436 " 
				+ mNickName + " " 
				+ newnick + " :" + Utils::make_string(mNickName, "Nick is in use.")
				+ config->EOFMessage);
		}
    }
}

void User::cmdUser(const std::string& ident) {
    if(bSentUser) {
        sendAsServer("462 " 
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
	setMode('w', true);
	sendAsServer("MODE " + mNickName + " +w" + config->EOFMessage);
	Servidor::sendall("UMODE " + mNickName + " +w");
	Servidor::sendall("WEBIRC " + mNickName + " " + ip);
}

void User::cmdAway(const std::string &away, bool on) {
	bAway = on;
	mAway = away;
	for (auto channel : mChannels) {
		if (channel->isonflood() == true && ChanServ::Access(mNickName, channel->name()) == 0) {
			sendAsServer("461 " + mNickName + " :" + Utils::make_string(mNickName, "The channel is on flood, you cannot speak.") + config->EOFMessage);
			continue;
		}
		channel->increaseflood();
		channel->broadcast_away(this, away, on);
	}
}

bool User::is_away() {
	return bAway;
}

std::string User::away_reason() {
	return mAway;
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

void User::SetLang(const std::string lang) {
	mLang = lang;
}

std::string User::GetLang() const {
	return mLang;
}

void User::cmdQuit() {
	Parser::log(Utils::make_string("", "Nick %s leaves irc", mNickName.c_str()));
	if (!bSentQuit)
		Exit();
	bProperlyQuit = true;
	if (getMode('o') == true)
		miRCOps.erase(this);
	if (LocalUser == true) {
		Servidor::sendall("QUIT " + mNickName);
		deadline_u.cancel();
		close();
	}
    Mainframe::instance()->removeUser(mNickName);
}

void User::cmdPart(Channel* channel) {
	Parser::log(Utils::make_string("", "Nick %s leaves channel: %s", nick().c_str(), channel->name().c_str()));
	channel->broadcast(messageHeader() + "PART " + channel->name() + config->EOFMessage);
	channel->removeUser(this);
	mChannels.erase(channel);
}

void User::cmdJoin(Channel* channel) {
	Parser::log(Utils::make_string("", "Nick %s joins channel: %s", nick().c_str(), channel->name().c_str()));
	mChannels.insert(channel);
	channel->addUser(this);
	channel->broadcast(messageHeader() + "JOIN :" + channel->name() + config->EOFMessage);
	channel->sendUserList(this);
}

void User::cmdKick(User* victim, const std::string& reason, Channel* channel) {
    channel->broadcast(":" + mNickName + " KICK " + channel->name() + " " + victim->nick() + " :" + reason + config->EOFMessage);
}

void User::cmdPing(const std::string &response) {
    sendAsServer("PONG " + config->Getvalue("serverName") + " :" + response + config->EOFMessage);
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

Ircv3 		User::iRCv3() const {
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

std::string User::cloak() {
	if (getMode('r') == true) {
		std::string cloak;
		if ((cloak = NickServ::GetvHost(mNickName)) != "")
			return cloak;
	}
	return mCloak;
}

bool User::connclose() {
	return (!bSentNick);
}

bool User::propquit() {
	return bProperlyQuit;
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
	for (auto channel : mChannels) {
		channel->broadcast_except_me(nick(), messageHeader() + "PART " + channel->name() + " :vHost" + config->EOFMessage);
		Servidor::sendall("SPART " + nick() + " " + channel->name());
		std::string mode = "+";
		if (channel->isOperator(this) == true)
			mode.append("o");
		else if (channel->isHalfOperator(this) == true)
			mode.append("h");
		else if (channel->isVoice(this) == true)
			mode.append("v");
		else
			mode.append("x");
			
		channel->broadcast_except_me(nick(), messageHeader() + "JOIN :" + channel->name() + config->EOFMessage);
		if (mode != "+x")
			channel->broadcast_except_me(nick(), ":" + config->Getvalue("chanserv") + " MODE " + channel->name() + " " + mode + " " + this->nick() + config->EOFMessage);
		Servidor::sendall("SJOIN " + nick() + " " + channel->name() + " " + mode);
	}
	sendAsServer("396 " + mNickName + " " + cloak() + " :is now your hidden host" + config->EOFMessage);
}

void User::propagatenick(const std::string &nickname) {
	for (auto channel : mChannels) {
		channel->broadcast(messageHeader() + "NICK " + nickname + config->EOFMessage);
	}
}

void User::SNICK(const std::string &nickname, const std::string &ident, const std::string &host, const std::string &cloak, std::string login, std::string modos) {
	mNickName = nickname;
	mIdent = ident;
	mHost = host;
	mCloak = cloak;
	bLogin = (time_t ) stoi(login);
	bSentNick = true;
	bSentMotd = true;
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
	bSentUser = true;
}

void User::SJOIN(Channel* channel) {
	Parser::log(Utils::make_string("", "Nick %s joins channel: %s", nick().c_str(), channel->name().c_str()));
    mChannels.insert(channel);
    channel->addUser(this);
    channel->broadcast(messageHeader() + "JOIN :" + channel->name() + config->EOFMessage);
}

void User::SKICK(Channel* channel) {
    channel->removeUser(this);
    mChannels.erase(channel);
}

void User::QUIT() {
    bProperlyQuit = true;
	Parser::log(Utils::make_string("", "Nick %s leaves irc", nick().c_str()));
	if (!bSentQuit)
		Exit();
	if (this->getMode('o') == true)
		miRCOps.erase(this);
	if (LocalUser == true) {
		deadline_u.cancel();
		close();
	}
    Mainframe::instance()->removeUser(mNickName);
}

void User::NETSPLIT() {
    bProperlyQuit = true;
	Parser::log(Utils::make_string("", "Nick %s leaves irc caused by a netsplit", nick().c_str()));
	if (!bSentQuit)
		Exit();
	if (this->getMode('o') == true)
		miRCOps.erase(this);
    Mainframe::instance()->removeUser(mNickName);
}

std::string User::messageHeader() {
	return std::string(":"+mNickName+"!"+mIdent+"@"+cloak()+" ");
}

int User::Channels() {
	return mChannels.size();
}

bool User::canchangenick() {
	if (Channels() == 0)
		return true;
	for (auto channel : mChannels) {
		if (getMode('o') == true)
			return true;
		if (ChanServ::IsRegistered(channel->name()) == true && ChanServ::HasMode(channel->name(), "NONICKCHANGE") == 1)
			return false;
		if (channel->isonflood() == true && this->getMode('o') == false)
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
		if (GetPing() + 200 < time(0) && LocalUser == true) {
			deadline_u.cancel();
			close();
		} else if (LocalUser == true) {
			send("PING :" + config->Getvalue("serverName") + config->EOFMessage);
			deadline_u.cancel();
			deadline_u.expires_from_now(boost::posix_time::seconds(90));
			deadline_u.async_wait(boost::bind(&User::check_ping, this, boost::asio::placeholders::error));
		}
	}
}

void User::NICK(const std::string &nickname) {
	Parser::log(Utils::make_string("", "Nickname %s changes nick to: %s with ip: %s", mNickName.c_str(), nickname.c_str(), mHost.c_str()));
	std::string oldheader = messageHeader();
	setNick(nickname);
	for (auto channel : mChannels) {
		channel->broadcast(oldheader + "NICK " + nickname + config->EOFMessage);
	}
}
