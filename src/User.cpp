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

#include "ZeusiRCd.h"
#include "Utils.h"
#include "Server.h"
#include "services.h"
#include "oper.h"
#include <mutex>

extern OperSet miRCOps;

std::mutex quit_mtx;
std::map<std::string, User *> Users;
std::map<std::string, Channel *> Channels;

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

void User::Exit(bool close) {
	const std::scoped_lock<std::mutex> lock(quit_mtx);
	if (quit == false)
	{
		quit = true;
		if (getMode('o') == true)
			miRCOps.erase(mNickName);
		if (getMode('r') == true)
			NickServ::UpdateLogin(this);
		if (bSentNick) {
		  MakeQuit();
		  Server::Send("QUIT " + mNickName);
		  Utils::log("El nick " + mNickName + " sale del chat");
		}
		if (close)
			Close();
	}
}

void User::MakeQuit()
{
    auto it = channels.begin();
    while (it != channels.end()) {
		Channel *chan = Channel::GetChannel((*it)->name);
		chan->quit(this);
		chan->broadcast(messageHeader() + "QUIT :QUIT");
		it = channels.erase(it);
    }
    std::string username = mNickName;
    std::transform(username.begin(), username.end(), username.begin(), ::tolower);
    Users.erase(username);
}

void User::SendAsServer(const std::string message)
{
	deliver(":"+config["serverName"].as<std::string>()+" "+message);
}

void User::QUIT() {
	MakeQuit();
	if (getMode('o') == true)
		miRCOps.erase(mNickName);
	std::string username = mNickName;
    std::transform(username.begin(), username.end(), username.begin(), ::tolower);
    Users.erase(username);
}

void User::Cycle() {
	if (channels.size() == 0)
		return;
	std::string vhost = NickServ::GetvHost(mNickName);
	std::string oldvhost = mvHost;
	if (!vhost.empty())
		mvHost = vhost;
	else if (NickServ::IsRegistered(mNickName) == false)
		mvHost = mCloak;
	if (mvHost == oldvhost)
		return;
	for (auto channel : channels) {
		channel->broadcast_except_me(mNickName, ":" + mNickName + "!" + mIdent + "@" + oldvhost + " PART " + channel->name + " :vHost");
		Server::Send("SPART " + mNickName + " " + channel->name);
		std::string mode = "+";
		if (channel->IsOperator(this) == true)
			mode.append("o");
		else if (channel->IsHalfOperator(this) == true)
			mode.append("h");
		else if (channel->IsVoice(this) == true)
			mode.append("v");
		else
			mode.append("x");
		channel->broadcast_except_me(mNickName, ":" + mNickName + "!" + mIdent + "@" + mvHost + " JOIN :" + channel->name);
		if (mode != "+x")
			channel->broadcast_except_me(mNickName, ":" + config["chanserv"].as<std::string>() + " MODE " + channel->name + " " + mode + " " + mNickName);
		Server::Send("SJOIN " + mNickName + " " + channel->name + " " + mode);
	}
	SendAsServer("396 " + mNickName + " " + mvHost + " :is now your hidden host");
}

User *User::GetUser(std::string user)
{
  if (FindUser(user) == false)
	return nullptr;
  std::transform(user.begin(), user.end(), user.begin(), ::tolower);
  auto usr = (*(Users.find(user)));
  return usr.second;
}

bool User::AddUser(User *user, std::string newnick)
{
  if (User::FindUser(newnick))
	return false;
  else
  {
	std::transform(newnick.begin(), newnick.end(), newnick.begin(), ::tolower);
    Users.insert(std::pair<std::string,User *>(newnick,user));
    return true;
  }
}

bool User::ChangeNickName(std::string oldnick, std::string newnick)
{
  if (User::FindUser(newnick))
    return false;
  else if (!User::FindUser(oldnick))
	return false;
  else
  {
    User *tmp = User::GetUser(oldnick);
    tmp->mNickName = newnick;
    std::transform(newnick.begin(), newnick.end(), newnick.begin(), ::tolower);
    AddUser(tmp, newnick);
	std::transform(oldnick.begin(), oldnick.end(), oldnick.begin(), ::tolower);
    Users.erase(oldnick);
    return true;
  }
}

bool User::FindUser(std::string nick)
{
  std::transform(nick.begin(), nick.end(), nick.begin(), ::tolower);
  return (Users.find(nick) != Users.end());
}

void User::SJOIN(Channel* channel) {
	channels.insert(channel);
	channel->InsertUser(this);
	channel->broadcast(messageHeader() + "JOIN " + channel->name);
}

void User::SPART(Channel* channel) {
	channel->broadcast(messageHeader() + "PART " + channel->name);
	channel->RemoveUser(this);
	channels.erase(channel);
	if (channel->users.size() == 0)
		delete Channel::GetChannel(channel->name);
}

void User::kick(std::string kicker, std::string victim, const std::string& reason, Channel* channel) {
    channel->broadcast(":" + kicker + " KICK " + channel->name + " " + victim + " :" + reason);
    channel->RemoveUser(this);
    channels.erase(channel);
}

void User::away(const std::string away, bool on) {
	bAway = on;
	mAway = away;
	for (auto channel : channels) {
		if (channel->isonflood() == true && ChanServ::Access(mNickName, channel->name) == 0) {
			SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The channel is on flood, you cannot speak."));
			continue;
		}
		channel->increaseflood();
		channel->broadcast_away(this, away, on);
	}
}

void User::NICK(const std::string nickname) {
	std::string oldheader = messageHeader();
	mNickName = nickname;
	for (auto channel : channels) {
		channel->broadcast(oldheader + "NICK " + nickname);
	}
}

bool User::canchangenick() {
	if (channels.size() == 0)
		return true;
	for (auto channel : channels) {
		if (getMode('o') == true)
			return true;
		if (ChanServ::IsRegistered(channel->name) == true && ChanServ::HasMode(channel->name, "NONICKCHANGE") == 1)
			return false;
		if (channel->isonflood() == true && getMode('o') == false)
			return false;
	}
	return true;
}
