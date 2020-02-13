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

void LocalUser::CheckPing() {
	if (bPing + 200 < time(0)) {
		if (quit == false) {
			quit = true;
			Close();
		}
	} else {
		SendAsServer("PING :" + config["serverName"].as<std::string>());
	}
};

void LocalUser::SendAsServer(const std::string message)
{
	Send(":"+config["serverName"].as<std::string>()+" "+message);
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
			channel->broadcast_except_me(mNickName, ":" + config["chanserv"].as<std::string>() + " MODE " + channel->mName + " " + mode + " " + mNickName);
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
			channel->broadcast_except_me(mNickName, ":" + config["chanserv"].as<std::string>() + " MODE " + channel->mName + " " + mode + " " + mNickName);
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
	channel->broadcast(messageHeader() + "JOIN :" + channel->name());
	channel->sendUserList(this);
}

void RemoteUser::QUIT() {
	MakeQuit();
	if (getMode('o') == true)
		miRCOps.erase(mNickName);
    Mainframe::instance()->removeRemoteUser(mNickName);
}

void RemoteUser::SJOIN(Channel* channel) {
	mChannels.insert(channel);
	channel->addUser(this);
	channel->broadcast(messageHeader() + "JOIN " + channel->name());
}

void RemoteUser::SPART(Channel* channel) {
	channel->broadcast(messageHeader() + "PART " + channel->name());
	mChannels.erase(channel);
	channel->removeUser(this);
}

void RemoteUser::NICK(const std::string &nickname) {
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

void LocalUser::Exit(bool close) {
	if (bSentNick)
		User::log("El nick " + mNickName + " sale del chat");
	MakeQuit();
	if (getMode('o') == true)
		miRCOps.erase(mNickName);
	if (getMode('r') == true)
		NickServ::UpdateLogin(this);
	if (close)
		Close();
	Server::Send("QUIT " + mNickName);
	Mainframe::instance()->removeLocalUser(mNickName);
}

void LocalUser::MakeQuit()
{
	const std::scoped_lock<std::mutex> lock(quit_mtx);
	for (auto channel : mChannels) {
		channel->removeUser(this);
		channel->broadcast(messageHeader() + "QUIT :QUIT");
	}
}

void RemoteUser::MakeQuit()
{
	const std::scoped_lock<std::mutex> lock(quit_mtx);
	for (auto channel : mChannels) {
		channel->removeUser(this);
		channel->broadcast(messageHeader() + "QUIT :QUIT");
	}
}
