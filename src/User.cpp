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

#include "Channel.h"
#include "mainframe.h"
#include "oper.h"
#include "sha256.h"
#include "Utils.h"
#include "services.h"

std::mutex quit_mtx;
extern OperSet miRCOps;
extern time_t encendido;

std::string& ltrim(std::string& str, const std::string& chars = "\t\n\v\f\r ");
std::string& rtrim(std::string& str, const std::string& chars = "\t\n\v\f\r ");
std::string& trim(std::string& str, const std::string& chars = "\t\n\v\f\r ");

void LocalUser::CheckNick() {
	if (!bSentNick) {
		quit = true;
		Close();
	}
};

void LocalUser::CheckPing() {
	if (bPing + 200 < time(0)) {
		quit = true;
		Close();
	} else {
		SendAsServer("PING :" + config->Getvalue("serverName"));
	}
};

void LocalUser::SendAsServer(const std::string message)
{
	Send(":"+config->Getvalue("serverName")+" "+message);
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

std::string User::messageHeader()
{
	return ":" + mNickName + "!" + mIdent + "@" + mvHost + " ";
}

void LocalUser::Cycle() {
	if (Channs() == 0)
		return;
	std::string vhost = NickServ::GetvHost(mNickName);
	std::string oldvhost = mvHost;
	if (!vhost.empty())
		mvHost = vhost;
	else if (NickServ::IsRegistered(mNickName) == false)
		mvHost = mCloak;
	if (mvHost == oldvhost)
		return;
	for (auto channel : mChannels) {
		channel->broadcast_except_me(mNickName, ":" + mNickName + "!" + mIdent + "@" + oldvhost + " PART " + channel->mName + " :vHost");
		Server::Send("SPART " + mNickName + " " + channel->mName);
		std::string mode = "+";
		if (channel->isOperator(this) == true)
			mode.append("o");
		else if (channel->isHalfOperator(this) == true)
			mode.append("h");
		else if (channel->isVoice(this) == true)
			mode.append("v");
		else
			mode.append("x");
		channel->broadcast_except_me(mNickName, ":" + mNickName + "!" + mIdent + "@" + mvHost + " JOIN :" + channel->mName);
		if (mode != "+x")
			channel->broadcast_except_me(mNickName, ":" + config->Getvalue("chanserv") + " MODE " + channel->mName + " " + mode + " " + mNickName);
		Server::Send("SJOIN " + mNickName + " " + channel->mName + " " + mode);
	}
	SendAsServer("396 " + mNickName + " " + mvHost + " :is now your hidden host");
}

void RemoteUser::Cycle() {
	std::string vhost = NickServ::GetvHost(mNickName);
	std::string oldvhost = mvHost;
	if (!vhost.empty())
		mvHost = vhost;
	else if (NickServ::IsRegistered(mNickName) == false)
		mvHost = mCloak;
	if (mvHost == oldvhost)
		return;
	for (auto channel : mChannels) {
		channel->broadcast_except_me(mNickName, ":" + mNickName + "!" + mIdent + "@" + oldvhost + " PART " + channel->mName + " :vHost");
		std::string mode = "+";
		if (channel->isOperator(this) == true)
			mode.append("o");
		else if (channel->isHalfOperator(this) == true)
			mode.append("h");
		else if (channel->isVoice(this) == true)
			mode.append("v");
		else
			mode.append("x");
		channel->broadcast_except_me(mNickName, ":" + mNickName + "!" + mIdent + "@" + mvHost + " JOIN :" + channel->mName);
		if (mode != "+x")
			channel->broadcast_except_me(mNickName, ":" + config->Getvalue("chanserv") + " MODE " + channel->mName + " " + mode + " " + mNickName);
	}
}

int LocalUser::Channs()
{
	return mChannels.size();
}

bool LocalUser::canchangenick() {
	if (Channs() == 0)
		return true;
	for (auto channel : mChannels) {
		if (getMode('o') == true)
			return true;
		if (ChanServ::IsRegistered(channel->name()) == true && ChanServ::HasMode(channel->name(), "NONICKCHANGE") == 1)
			return false;
		if (channel->isonflood() == true && getMode('o') == false)
			return false;
	}
	return true;
}

void LocalUser::cmdNick(const std::string& newnick) {
    if(bSentNick) {
        if(Mainframe::instance()->changeLocalNickname(mNickName, newnick)) {
			User::log(Utils::make_string("", "Nickname %s changes nick to: %s with ip: %s", mNickName.c_str(), newnick.c_str(), mHost.c_str()));
            Send(messageHeader() + "NICK " + newnick);
			Server::Send("NICK " + mNickName + " " + newnick);
			std::string oldheader = messageHeader();
			std::string oldnick = mNickName;
			mNickName = newnick;
            for (auto channel : mChannels) {
                channel->broadcast_except_me(mNickName, oldheader + "NICK " + newnick);
                ChanServ::CheckModes(this, channel->name());
            }
			mCloak = sha256(mHost).substr(0, 16);
            if (getMode('r') == true)
				NickServ::checkmemos(this);
            if (OperServ::IsOper(newnick) == true && getMode('o') == false) {
				miRCOps.insert(mNickName);
				setMode('o', true);
				SendAsServer("MODE " + mNickName + " +o");
				Server::Send("UMODE " + mNickName + " +o");
			} else if (getMode('o') == true && OperServ::IsOper(newnick) == false) {
				miRCOps.erase(mNickName);
				setMode('o', false);
				SendAsServer("MODE " + mNickName + " -o");
				Server::Send("UMODE " + mNickName + " -o");
			}
			Cycle();
        } else {
            SendAsServer("436 " 
				+ mNickName + " " 
				+ newnick + " :" + Utils::make_string(mLang, "Nick is in use."));
        }
    } else {
		if (Mainframe::instance()->addLocalUser(this, newnick)) {
			Send(messageHeader() + "NICK :" + newnick);
			mNickName = newnick;
			mCloak = sha256(mHost).substr(0, 16);
			std::string vhost = NickServ::GetvHost(mNickName);
			if (!vhost.empty())
				mvHost = vhost;
			else
				mvHost = mCloak;
			User::log(Utils::make_string("", "Nickname %s enter to irc with ip: %s", newnick.c_str(), mHost.c_str()));
			bPing = time(0);
			bLogin = time(0);
			bSentNick = true;

			if(bSentNick && !bSentMotd) {
				bSentMotd = true;
				
				struct tm tm;
				localtime_r(&encendido, &tm);
				char date[32];
				strftime(date, sizeof(date), "%r %d-%m-%Y", &tm);
				std::string fecha = date;
				SendAsServer("001 " + mNickName + " :" + Utils::make_string(mLang, "Welcome to \002%s.\002", config->Getvalue("network").c_str()));
				SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "Your server is: %s working with: %s", config->Getvalue("serverName").c_str(), config->version.c_str()));
				SendAsServer("003 " + mNickName + " :" + Utils::make_string(mLang, "This server was created: %s", fecha.c_str()));
				SendAsServer("004 " + mNickName + " " + config->Getvalue("serverName") + " " + config->version + " rzoiws robtkmlvshn r");
				SendAsServer("005 " + mNickName + " NETWORK=" + config->Getvalue("network") + " are supported by this server");
				SendAsServer("005 " + mNickName + " NICKLEN=" + config->Getvalue("nicklen") + " MAXCHANNELS=" + config->Getvalue("maxchannels") + " CHANNELLEN=" + config->Getvalue("chanlen") + " are supported by this server");
				SendAsServer("005 " + mNickName + " PREFIX=(ohv)@%+ STATUSMSG=@%+ are supported by this server");
				SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "There are \002%s\002 users and \002%s\002 channels.", std::to_string(Mainframe::instance()->countusers()).c_str(), std::to_string(Mainframe::instance()->countchannels()).c_str()));
				SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "There are \002%s\002 registered nicks and \002%s\002 registered channels.", std::to_string(NickServ::GetNicks()).c_str(), std::to_string(ChanServ::GetChans()).c_str()));
				SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "There are \002%s\002 connected iRCops.", std::to_string(Oper::Count()).c_str()));
				SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "There are \002%s\002 connected servers.", std::to_string(Server::count()).c_str()));
				SendAsServer("422 " + mNickName + " :No MOTD");
				if (dynamic_cast<LocalSSLUser*>(this) != nullptr) {
					setMode('z', true);
					SendAsServer("MODE " + mNickName + " +z");
				} if (dynamic_cast<LocalWebUser*>(this) != nullptr) {
					setMode('w', true);
					SendAsServer("MODE " + mNickName + " +w");
				} if (OperServ::IsOper(mNickName) == true) {
					miRCOps.insert(mNickName);
					setMode('o', true);
					SendAsServer("MODE " + mNickName + " +o");
				}
				SendAsServer("396 " + mNickName + " " + mvHost + " :is now your hidden host");
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
				Server::Send("SNICK " + mNickName + " " + identi + " " + mHost + " " + mCloak + " " + mvHost + " " + std::to_string(bLogin) + " " + mServer + " " + modos);
				if (getMode('r') == true)
					NickServ::checkmemos(this);
			}
		} else {
			SendAsServer("436 " 
				+ mNickName + " " 
				+ newnick + " :" + Utils::make_string(mLang, "Nick is in use."));
		}
    }
}

void LocalUser::cmdPart(Channel* channel) {
	User::log(Utils::make_string("", "Nick %s leaves channel: %s", mNickName.c_str(), channel->name().c_str()));
	channel->broadcast(messageHeader() + "PART " + channel->name());
	channel->removeUser(this);
	mChannels.erase(channel);
}

void LocalUser::cmdJoin(Channel* channel) {
	User::log(Utils::make_string("", "Nick %s joins channel: %s", mNickName.c_str(), channel->name().c_str()));
	mChannels.insert(channel);
	channel->addUser(this);
	channel->broadcast(messageHeader() + "JOIN " + channel->name());
	channel->sendUserList(this);
}

void RemoteUser::QUIT() {
	quit_mtx.lock();
	User::log(Utils::make_string("", "Nick %s leaves irc", mNickName.c_str()));
	for (auto channel : mChannels) {
		channel->broadcast(messageHeader() + "QUIT :QUIT");
		channel->removeUser(this);
	}
	if (getMode('o') == true)
		miRCOps.erase(mNickName);
    Mainframe::instance()->removeRemoteUser(mNickName);
	quit_mtx.unlock();
}

void RemoteUser::SJOIN(Channel* channel) {
	User::log(Utils::make_string("", "Nick %s joins channel: %s", mNickName.c_str(), channel->name().c_str()));
	mChannels.insert(channel);
	channel->addUser(this);
	channel->broadcast(messageHeader() + "JOIN " + channel->name());
}

void RemoteUser::SPART(Channel* channel) {
	User::log(Utils::make_string("", "Nick %s parts channel: %s", mNickName.c_str(), channel->name().c_str()));
	channel->broadcast(messageHeader() + "PART " + channel->name());
	mChannels.erase(channel);
	channel->removeUser(this);
}

void RemoteUser::NICK(const std::string &nickname) {
	User::log(Utils::make_string("", "Nickname %s changes nick to: %s with ip: %s", mNickName.c_str(), nickname.c_str(), mHost.c_str()));
	std::string oldheader = messageHeader();
	mNickName = nickname;
	for (auto channel : mChannels) {
		channel->broadcast(oldheader + "NICK " + nickname);
	}
}

void RemoteUser::SKICK(std::string kicker, std::string victim, const std::string reason, Channel* channel) {
	channel->broadcast(":" + kicker + " KICK " + channel->name() + " " + victim + " :" + reason);
    channel->removeUser(this);
    mChannels.erase(channel);
}

void LocalUser::cmdKick(std::string kicker, std::string victim, const std::string& reason, Channel* channel) {
    channel->broadcast(":" + kicker + " KICK " + channel->name() + " " + victim + " :" + reason);
    channel->removeUser(this);
    mChannels.erase(channel);
}


void LocalUser::cmdAway(const std::string &away, bool on) {
	bAway = on;
	mAway = away;
	for (auto channel : mChannels) {
		if (channel->isonflood() == true && ChanServ::Access(mNickName, channel->name()) == 0) {
			SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The channel is on flood, you cannot speak."));
			continue;
		}
		channel->increaseflood();
		channel->broadcast_away(this, away, on);
	}
}

void LocalUser::SKICK(Channel* channel) {
    channel->removeUser(this);
    mChannels.erase(channel);
}

void LocalUser::sendCAP(const std::string &cmd) {
	negotiating = true;
	if (config->Getvalue("ircv3") == "true")
		SendAsServer("CAP * " + cmd + " :away-notify userhost-in-names" + sts());
}

void LocalUser::Request(std::string request) {
	std::string capabs = ":";
	std::string req = request.substr(9);
	std::vector<std::string>  x;
	Config::split(req, x, " \t");
	for (unsigned int i = 0; i < x.size(); i++) {
		if (x[i] == "away-notify") {
			capabs.append(x[i] + " ");
			away_notify = true;
		} else if (x[i] == "userhost-in-names") {
			capabs.append(x[i] + " ");
			userhost_in_names = true;
		} else if (x[i] == "extended-join") {
			capabs.append(x[i] + " ");
			extended_join = true;
		}
	}
	trim(capabs);
	SendAsServer("CAP * ACK " + capabs);
}

std::string LocalUser::sts() {
	int puerto = 0;
	if (mHost.find(":") != std::string::npos) {
		for (unsigned int i = 0; config->Getvalue("listen6["+std::to_string(i)+"]ip").length() > 0; i++) {
			if (config->Getvalue("listen6["+std::to_string(i)+"]class") == "client" &&
				(config->Getvalue("listen6["+std::to_string(i)+"]ssl") == "1" || config->Getvalue("listen6["+std::to_string(i)+"]ssl") == "true"))
				puerto = (int) stoi(config->Getvalue("listen6["+std::to_string(i)+"]port"));
		}
	} else {
		for (unsigned int i = 0; config->Getvalue("listen["+std::to_string(i)+"]ip").length() > 0; i++) {
			if (config->Getvalue("listen["+std::to_string(i)+"]class") == "client" &&
				(config->Getvalue("listen["+std::to_string(i)+"]ssl") == "1" || config->Getvalue("listen["+std::to_string(i)+"]ssl") == "true"))
				puerto = (int) stoi(config->Getvalue("listen["+std::to_string(i)+"]port"));
		}
	}
	if (puerto == 0)
		return "";
	else
		return " sts=port=" + std::to_string(puerto) + ",duration=10";
}

void LocalUser::recvEND() {
	negotiating = false;
}

void LocalUser::Exit() {
	if (!bSentQuit)
	{
		bSentQuit = true;
		User::log("El nick " + mNickName + " sale del chat");
		quit_mtx.lock();
		for (auto channel : mChannels) {
			channel->broadcast(messageHeader() + "QUIT :QUIT");
			channel->removeUser(this);
		}
		quit_mtx.unlock();
		if (getMode('o') == true)
			miRCOps.erase(mNickName);
		Server::Send("QUIT " + mNickName);
		Mainframe::instance()->removeLocalUser(mNickName);
	}
}
