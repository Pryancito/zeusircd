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
#include "services.h"

void Channel::part(User *user)
{
  broadcast(user->messageHeader() + "PART " + name);
  std::string username = user->mNickName;
  std::transform(username.begin(), username.end(), username.begin(), ::tolower);
  auto usr = (*(Users.find(username)));
  auto it = usr.second->channels.find (this);
  *(usr.second->channels).erase(it);
  RemoveUser(user);
  Utils::log(Utils::make_string("", "Nick %s leaves channel: %s", user->mNickName.c_str(), name.c_str()));
  if (users.size() == 0) {
	std::string nombre = name;
	std::transform(nombre.begin(), nombre.end(), nombre.begin(), ::tolower);
	delete Channel::GetChannel(nombre);
    Channels.erase(nombre);
  }
}

void Channel::join(User *user)
{
  std::string username = user->mNickName;
  std::transform(username.begin(), username.end(), username.begin(), ::tolower);
  auto usr = (*(Users.find(username)));
  usr.second->channels.insert(this);
  InsertUser(user);
  broadcast(user->messageHeader() + "JOIN " + name);
  send_userlist(user);
  Utils::log(Utils::make_string("", "Nick %s joins channel: %s", user->mNickName.c_str(), name.c_str()));
}

void Channel::quit(User *user)
{
  broadcast(user->messageHeader() + "QUIT :QUIT");
  std::string username = user->mNickName;
  std::transform(username.begin(), username.end(), username.begin(), ::tolower);
  auto usr = (*(Users.find(username)));
  auto it = usr.second->channels.find (this);
  *(usr.second->channels).erase(it);
  RemoveUser(user);
  Utils::log(Utils::make_string("", "Nick %s quits irc: %s", user->mNickName.c_str(), name.c_str()));
  if (users.size() == 0) {
	std::string nombre = name;
	std::transform(nombre.begin(), nombre.end(), nombre.begin(), ::tolower);
	delete Channel::GetChannel(nombre);
    Channels.erase(nombre);
  }
}

void Channel::broadcast(const std::string message) {
	for (auto *user : users) {
		if (user->is_local == true)
			user->deliver(message);
	}
}

void Channel::broadcast_except_me(const std::string nickname, const std::string message) {
	for (auto *user : users) {
		if (user->mNickName != nickname && user->is_local == true)
			user->deliver(message);
	}
}

void Channel::broadcast_away(User *user, std::string away, bool on) {
	for (auto *usr : users) {
		if (usr->away_notify == true && on) {
			usr->deliver(user->messageHeader() + "AWAY " + away);
		} else if (usr->away_notify == true && !on) {
			usr->deliver(user->messageHeader() + "AWAY");
		} if (on) {
			usr->deliver(user->messageHeader() + "NOTICE " + name + " :AWAY ON " + away);
		} else {
			usr->deliver(user->messageHeader() + "NOTICE " + name + " :AWAY OFF");
		}
	}
}

bool Channel::FindChannel(std::string nombre)
{
  std::transform(nombre.begin(), nombre.end(), nombre.begin(), ::tolower);
  return (Channels.find(nombre) != Channels.end());
}

Channel *Channel::GetChannel(std::string chan)
{
  if (FindChannel(chan) == false)
	return nullptr;
  std::transform(chan.begin(), chan.end(), chan.begin(), ::tolower);
  auto channel = (*(Channels.find(chan)));
  return channel.second;
}

bool Channel::HasUser(User *user)
{
  return (users.find(user) != users.end());
}

void Channel::InsertUser(User *user)
{
  if (HasUser(user) == false)
    users.insert(user);
}

void Channel::RemoveUser(User *user)
{
  if (HasUser(user) == true)
  {
	auto it = users.find(user);
    users.erase(it);
  }
  if (IsOperator(user) == true)
  {
    auto it = operators.find(user);
    operators.erase(it);
  }
  if (IsHalfOperator(user) == true)
  {
    auto it = halfoperators.find(user);
    halfoperators.erase(it);
  }
  if (IsVoice(user) == true)
  {
    auto it = voices.find(user);
    voices.erase(it);
  }
}

void Channel::GiveOperator(User *user)
{
  if (IsOperator(user) == false)
    operators.insert(user);
}

bool Channel::IsOperator(User *user)
{
  return (operators.find(user) != operators.end());
}

void Channel::RemoveOperator(User *user)
{
  if (IsOperator(user) == true)
  {
	auto it = operators.find(user);
    operators.erase(it);
  }
}

void Channel::GiveHalfOperator(User *user)
{
  if (IsHalfOperator(user) == false)
    halfoperators.insert(user);
}

bool Channel::IsHalfOperator(User *user)
{
  return (halfoperators.find(user) != halfoperators.end());
}

void Channel::RemoveHalfOperator(User *user)
{
  if (IsHalfOperator(user) == true)
  {
	auto it = halfoperators.find(user);
    halfoperators.erase(it);
  }
}

void Channel::GiveVoice(User *user)
{
  if (IsVoice(user) == false)
    voices.insert(user);
}

bool Channel::IsVoice(User *user)
{
  return (voices.find(user) != voices.end());
}

void Channel::RemoveVoice(User *user)
{
  if (IsVoice(user) == true)
  {
	auto it = voices.find(user);
    voices.erase(it);
  }
}

bool Channel::getMode(char mode) {
	switch (mode) {
		case 'r': return mode_r;
		default: return false;
	}
	return false;
}

void Channel::setMode(char mode, bool option) {
	switch (mode) {
		case 'r': mode_r = option; break;
		default: break;
	}
	return;
}

void Ban::ExpireBan(const boost::system::error_code &e) {
	if (!e)
	{
		Channel* chan = Channel::GetChannel(canal);
		if (chan) {
			chan->broadcast(":" + config["chanserv"].as<std::string>() + " MODE " + chan->name + " -b " + this->mask());
			Server::Send("CMODE " + config["chanserv"].as<std::string>() + " " + chan->name + " -b " + this->mask());
			chan->UnBan(this);
		}
	}
}

std::string Ban::mask() {
	return mascara;
}

std::string Ban::whois() {
	return who;
}

time_t Ban::time() {
	return fecha;
}

void Channel::UnBan(Ban *ban) {
	bans.erase(ban);
	delete ban;
}

std::string pBan::mask() {
	return mascara;
}

std::string pBan::whois() {
	return who;
}

time_t pBan::time() {
	return fecha;
}

void Channel::UnpBan(pBan *ban) {
	pbans.erase(ban);
	delete ban;
}

bool Channel::IsBan(std::string mask) {
	if (users.size() == 0)
		return false;
	std::transform(mask.begin(), mask.end(), mask.begin(), ::tolower);
	for (auto ban : bans) {
		std::string bans = ban->mask();
		std::transform(bans.begin(), bans.end(), bans.begin(), ::tolower);
		if (Utils::Match(bans.c_str(), mask.c_str()) == true)
			return true;
	} for (auto ban : pbans) {
		std::string bans = ban->mask();
		std::transform(bans.begin(), bans.end(), bans.begin(), ::tolower);
		if (Utils::Match(bans.c_str(), mask.c_str()) == true)
			return true;
	}
	return false;
}

void Channel::resetflood() {
	flood = 0;
	is_flood = false;
	broadcast(":" + config["chanserv"].as<std::string>()
		+ " NOTICE "
		+ name + " :" + Utils::make_string("", "The channel has leaved the flood mode."));
	Server::Send("NOTICE " + config["chanserv"].as<std::string>() + " " + name + " :" + Utils::make_string("", "The channel has leaved the flood mode."));
}

void Channel::increaseflood() {
	if (ChanServ::IsRegistered(name) == true && ChanServ::HasMode(name, "FLOOD"))
		flood++;
	else
		return;
	if (flood >= ChanServ::HasMode(name, "FLOOD") && flood != 0 && is_flood == false) {
		broadcast(":" + config["chanserv"].as<std::string>()
			+ " NOTICE "
			+ name + " :" + Utils::make_string("", "The channel has entered into flood mode. The actions are restricted."));
		Server::Send("NOTICE " + config["chanserv"].as<std::string>() + " " + name + " :" + Utils::make_string("", "The channel has entered into flood mode. The actions are restricted."));
		is_flood = true;
	}
}

bool Channel::isonflood() {
	return is_flood;
}

void Channel::ExpireFlood() {
	if (is_flood == true)
		resetflood();
	else
		flood = 0;
}

void Channel::send_userlist(User* user) {
	std::string names = "";
	for (auto *usr : users) {
		std::string nickname = usr->mNickName;
		if (user->userhost_in_names)
			nickname = usr->mNickName + "!" + usr->mIdent + "@" + usr->mvHost;
		if(IsOperator(usr) == true) {
			if (!names.empty())
				names.append(" ");
			names.append("@" + nickname);
		} else if (IsHalfOperator(usr) == true) {
			if (!names.empty())
				names.append(" ");
			names.append("%" + nickname);
		} else if (IsVoice(usr) == true) {
			if (!names.empty())
				names.append(" ");
			names.append("+" + nickname);
		} else {
			if (!names.empty())
				names.append(" ");
			names.append(nickname);
		}
		if (names.length() > 500) {
			user->SendAsServer("353 " + user->mNickName + " = "  + name + " :" + names);
			names.clear();
		}
	}
	if (!names.empty())
		user->SendAsServer("353 " + user->mNickName + " = "  + name + " :" + names);

	user->SendAsServer("366 " + user->mNickName + " "  + name + " :" + Utils::make_string(user->mLang, "End of /NAMES list."));
}

void Channel::setBan(std::string mask, std::string whois) {
	Ban *ban = new Ban(name, mask, whois, time(0));
	bans.insert(ban);
}

void Channel::setpBan(std::string mask, std::string whois) {
	pBan *ban = new pBan(name, mask, whois, time(0));
	pbans.insert(ban);
}

void Channel::SBAN(std::string mask, std::string whois, std::string time) {
	time_t tiempo = (time_t ) stoi(time);
	Ban *ban = new Ban(name, mask, whois, tiempo);
	bans.insert(ban);
}

void Channel::SPBAN(std::string mask, std::string whois, std::string time) {
	time_t tiempo = (time_t ) stoi(time);
	pBan *ban = new pBan(name, mask, whois, tiempo);
	pbans.insert(ban);
}

void Channel::send_who_list(User* user) {
	for (auto *usr : users) {
		std::string oper = "";
		std::string away = "H";
		if (usr->getMode('o') == true)
			oper = "*";
		if (usr->bAway == true)
			away = "G";
		if(IsOperator(usr) == true) {
			user->SendAsServer("352 "
				+ usr->mNickName + " " 
				+ name + " " 
				+ usr->mNickName + " " 
				+ usr->mvHost + " " 
				+ "*.* " 
				+ usr->mNickName + " " + away + oper + "@ :0 " 
				+ "ZeusiRCd");
		} else if(IsHalfOperator(usr) == true) {
			user->SendAsServer("352 "
				+ usr->mNickName + " " 
				+ name + " " 
				+ usr->mNickName + " " 
				+ usr->mvHost + " " 
				+ "*.* " 
				+ usr->mNickName + " " + away + oper + "% :0 " 
				+ "ZeusiRCd");
		} else if(IsVoice(usr) == true) {
			user->SendAsServer("352 "
				+ usr->mNickName + " " 
				+ name + " " 
				+ usr->mNickName + " " 
				+ usr->mvHost + " " 
				+ "*.* " 
				+ usr->mNickName + " " + away + oper + "+ :0 " 
				+ "ZeusiRCd");
		} else {
			user->SendAsServer("352 "
				+ usr->mNickName + " " 
				+ name + " " 
				+ usr->mNickName + " " 
				+ usr->mvHost + " " 
				+ "*.* " 
				+ usr->mNickName + " " + away + oper + " :0 " 
				+ "ZeusiRCd");
		}
	}
	user->SendAsServer("315 " 
		+ user->mNickName + " " 
		+ name + " :" + "End of /WHO list.");
}
