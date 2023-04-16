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
#include "module.h"
#include "services.h"
#include "Utils.h"
#include "oper.h"
#include "sha256.h"
#include "Server.h"

extern time_t encendido;
extern OperSet miRCOps;
extern std::map<std::string, unsigned int> bForce;

class CMD_Nick : public Module
{
	public:
	CMD_Nick() : Module("NICK", 50, false) {};
	~CMD_Nick() {};
	virtual void command(User *user, std::string message) override {
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		if (results.size() < 2) {
			user->SendAsServer("431 " + user->mNickName + " :" + Utils::make_string(user->mLang, "No nickname: [ /nick yournick ]"));
			return;
		}
		
		if (results[1][0] == ':')
			results[1] = results[1].substr(1, results[1].length());

		if (OperServ::IsSpam(results[1], "N") == true && OperServ::IsSpam(results[1], "E") == false) {
			user->SendAsServer("432 " + results[1] + " :" + Utils::make_string(user->mLang, "Your nick is marked as SPAM."));
			return;
		}

		std::string nickname = results[1];
		std::string password = "";
	
		if (nickname.find("!") != std::string::npos || nickname.find(":") != std::string::npos) {
			std::vector<std::string> nickpass;
			Utils::split(nickname, nickpass, ":!");
			nickname = nickpass[0];
			if (nickpass[1].length() > 0)
				password = nickpass[1];
		} else if (!user->PassWord.empty())
			password = user->PassWord;

		if (Utils::checknick(nickname) == false) {
			user->SendAsServer("432 " + nickname + " :" + Utils::make_string(user->mLang, "The nick contains no-valid characters."));
			return;
		}

		if (strcasecmp(nickname.c_str(), "zeusircd") == 0) {
			user->SendAsServer("432 " + results[1] + " :" + Utils::make_string(user->mLang, "Reserved NickName."));
			return;
		}

		if ((bForce.find(nickname)) != bForce.end()) {
			if (bForce.count(nickname) >= 7) {
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + nickname + " :" + Utils::make_string(user->mLang, "Too much identify attempts for this nick. Try in 1 hour."));
				return;
			}
		}

		if (user->bSentNick)
		{
			if (user->canchangenick() == false) {
				user->SendAsServer("432 " + user->mNickName + " :" + Utils::make_string(user->mLang, "You cannot change your nick."));
				return;
			} else if (strcmp(nickname.c_str(), user->mNickName.c_str()) == 0) {
				return;
			} else if (strcasecmp(nickname.c_str(), user->mNickName.c_str()) == 0) {
				cmdNick(user, nickname);
				return;
			} else if (NickServ::Login(nickname, password) == true) {
				bForce[nickname] = 0;
				if (User::FindUser(nickname) == true)
				{
				  User *u = User::GetUser(nickname);
				  if (u->is_local == true) {
				    u->Exit(true);
				  } else {
				    u->QUIT();
				    Server::Send("QUIT " + u->mNickName);
				  }
				}
				if (user->getMode('r') == false) {
					user->setMode('r', true);
					user->SendAsServer("MODE " + nickname + " +r");
					Server::Send("UMODE " + nickname + " +r");
				}
				std::string lang = NickServ::GetLang(nickname);
				if (lang != "")
					user->mLang = lang;
				cmdNick(user, nickname);
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + nickname + " :" + Utils::make_string(user->mLang, "Welcome home."));
				NickServ::UpdateLogin(user);
				return;
			} else if (NickServ::IsRegistered(nickname) == true) {
				if (password == "") {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + nickname + " :" + Utils::make_string(user->mLang, "You need a password: [ /nick yournick:yourpass ]"));
					return;
				}
				else if (bForce.count(nickname) > 0) {
					bForce[nickname]++;
					Utils::log(Utils::make_string("", "Wrong password for nick: %s entered password: %s", nickname.c_str(), password.c_str()));
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + nickname + " :" + Utils::make_string(user->mLang, "Wrong password."));
					return;
				} else {
					bForce[nickname] = 1;
					Utils::log(Utils::make_string("", "Wrong password for nick: %s entered password: %s", nickname.c_str(), password.c_str()));
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + nickname + " :" + Utils::make_string(user->mLang, "Wrong password."));
					return;
				}
			} else if (User::FindUser(nickname)) {
				user->SendAsServer("433 " + nickname + " " + nickname + " :" + Utils::make_string(user->mLang, "The nick is used by somebody."));
				return;
			} else {
				if (user->getMode('r') == true) {
					user->setMode('r', false);
					user->SendAsServer("MODE " + user->mNickName + " -r");
					Server::Send("UMODE " + user->mNickName + " -r");
				}
				cmdNick(user, nickname);
			}
		}
		else
		{
			if (NickServ::Login(nickname, password) == true) {
				bForce[nickname] = 0;
				if (User::FindUser(nickname) == true)
				{
				  User *u = User::GetUser(nickname);
				  if (u->is_local == true) {
				    u->Exit(true);
				  } else {
				    u->QUIT();
				    Server::Send("QUIT " + u->mNickName);
				  }
				}
				if (user->getMode('r') == false) {
					user->setMode('r', true);
					user->SendAsServer("MODE " + nickname + " +r");
					Server::Send("UMODE " + nickname + " +r");
				}
				std::string lang = NickServ::GetLang(nickname);
				if (lang != "")
					user->mLang = lang;
				cmdNick(user, nickname);
				user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + nickname + " :" + Utils::make_string(user->mLang, "Welcome home."));
				NickServ::UpdateLogin(user);
				return;
			} else if (NickServ::IsRegistered(nickname) == true) {
				if (password == "") {
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + nickname + " :" + Utils::make_string(user->mLang, "You need a password: [ /nick yournick:yourpass ]"));
					return;
				}
				else if (bForce.count(nickname) > 0) {
					bForce[nickname]++;
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + nickname + " :" + Utils::make_string(user->mLang, "Wrong password."));
					return;
				} else {
					bForce[nickname] = 1;
					user->deliver(":" + config["nickserv"].as<std::string>() + " NOTICE " + nickname + " :" + Utils::make_string(user->mLang, "Wrong password."));
					return;
				}
			} else if (User::FindUser(nickname)) {
				user->SendAsServer("433 " + nickname + " " + nickname + " :" + Utils::make_string(user->mLang, "The nick is used by somebody."));
				return;
			} else {
				if (user->getMode('r') == true) {
					user->setMode('r', false);
					user->SendAsServer("MODE " + user->mNickName + " -r");
					Server::Send("UMODE " + user->mNickName + " -r");
				}
				cmdNick(user, nickname);
			}
		}
	}
	
	void cmdNick(User *user, const std::string& newnick) {
		if(user->bSentNick) {
			if (strcasecmp(newnick.c_str(), user->mNickName.c_str()) == 0) {
				user->deliver(user->messageHeader() + "NICK " + newnick);
				Server::Send("NICK " + user->mNickName + " " + newnick);
				std::string oldheader = user->messageHeader();
				std::string oldnick = user->mNickName;
				user->mNickName = newnick;
				User::ChangeNickName(oldnick, newnick);
				for (auto channel : user->channels) {
					channel->broadcast_except_me(newnick, oldheader + "NICK " + newnick);
					ChanServ::CheckModes(user, channel->name);
				}
			} else if(User::FindUser(newnick) == false) {
				Utils::log(Utils::make_string("", "Nickname %s changes nick to: %s with ip: %s", user->mNickName.c_str(), newnick.c_str(), user->mHost.c_str()));
				user->deliver(user->messageHeader() + "NICK " + newnick);
				Server::Send("NICK " + user->mNickName + " " + newnick);
				std::string oldheader = user->messageHeader();
				std::string oldnick = user->mNickName;
				user->mNickName = newnick;
				User::ChangeNickName(oldnick, newnick);
				for (auto channel : user->channels) {
					channel->broadcast_except_me(newnick, oldheader + "NICK " + newnick);
					ChanServ::CheckModes(user, channel->name);
				}
				user->mCloak = sha256(user->mHost).substr(0, 16);
				if (user->getMode('r') == true)
					NickServ::checkmemos(user);
				if (OperServ::IsOper(newnick) == true && user->getMode('o') == false) {
					miRCOps.insert(newnick);
					user->setMode('o', true);
					user->SendAsServer("MODE " + newnick + " +o");
					Server::Send("UMODE " + newnick + " +o");
				} else if (user->getMode('o') == true && OperServ::IsOper(newnick) == false) {
					miRCOps.erase(newnick);
					user->setMode('o', false);
					user->SendAsServer("MODE " + newnick + " -o");
					Server::Send("UMODE " + newnick + " -o");
				}
				user->Cycle();
			} else {
				user->SendAsServer("436 "	+ newnick + " "	+ newnick + " :" + Utils::make_string(user->mLang, "Nick is in use."));
			}
		} else {
			if (User::AddUser(user, newnick)) {
				user->is_local = true;
				user->deliver(user->messageHeader() + "NICK :" + newnick);
				user->mNickName = newnick;
				user->mCloak = sha256(user->mHost).substr(0, 16);
				std::string vhost = NickServ::GetvHost(newnick);
				if (!vhost.empty())
					user->mvHost = vhost;
				else
					user->mvHost = user->mCloak;
				Utils::log(Utils::make_string("", "Nickname %s enter to irc with ip: %s", newnick.c_str(), user->mHost.c_str()));
				user->bLogin = time(0);
				user->bSentNick = true;

				if(!user->bSentMotd) {
					user->bSentMotd = true;
					
					struct tm tm;
					localtime_r(&encendido, &tm);
					char date[32];
					strftime(date, sizeof(date), "%r %d-%m-%Y", &tm);
					std::string fecha = date;
					user->SendAsServer("001 " + user->mNickName + " :" + Utils::make_string(user->mLang, "Welcome to \002%s.\002", config["network"].as<std::string>().c_str()));
					user->SendAsServer("002 " + user->mNickName + " :" + Utils::make_string(user->mLang, "Your server is: %s working with: %s", config["serverName"].as<std::string>().c_str(), config["version"].as<std::string>().c_str()));
					user->SendAsServer("003 " + user->mNickName + " :" + Utils::make_string(user->mLang, "This server was created: %s", fecha.c_str()));
					user->SendAsServer("004 " + user->mNickName + " " + config["serverName"].as<std::string>() + " " + config["version"].as<std::string>() + " rzow robBvh r");
					user->SendAsServer("005 " + user->mNickName + " NETWORK=" + config["network"].as<std::string>() + " are supported by this server");
					user->SendAsServer("005 " + user->mNickName + " NICKLEN=" + config["nicklen"].as<std::string>() + " MAXCHANNELS=" + config["maxchannels"].as<std::string>() + " CHANNELLEN=" + config["chanlen"].as<std::string>() + " are supported by this server");
					user->SendAsServer("005 " + user->mNickName + " PREFIX=(ohv)@%+ STATUSMSG=@%+ are supported by this server");
					user->SendAsServer("002 " + user->mNickName + " :" + Utils::make_string(user->mLang, "There are \002%s\002 users and \002%s\002 channels.", std::to_string(Users.size()).c_str(), std::to_string(Channels.size()).c_str()));
					user->SendAsServer("002 " + user->mNickName + " :" + Utils::make_string(user->mLang, "There are \002%s\002 registered nicks and \002%s\002 registered channels.", std::to_string(NickServ::GetNicks()).c_str(), std::to_string(ChanServ::GetChans()).c_str()));
					user->SendAsServer("002 " + user->mNickName + " :" + Utils::make_string(user->mLang, "There are \002%s\002 connected iRCops.", std::to_string(Oper::Count()).c_str()));
					user->SendAsServer("002 " + user->mNickName + " :" + Utils::make_string(user->mLang, "There are \002%s\002 connected servers.", std::to_string(Server::count()).c_str()));
					user->SendAsServer("422 " + user->mNickName + " :No MOTD");
					if (user->getMode('z') == true) {
						user->SendAsServer("MODE " + user->mNickName + " +z");
					} if (user->getMode('w') == true) {
						user->SendAsServer("MODE " + user->mNickName + " +w");
					} if (OperServ::IsOper(user->mNickName) == true) {
						miRCOps.insert(user->mNickName);
						user->setMode('o', true);
						user->SendAsServer("MODE " + user->mNickName + " +o");
					}
					user->SendAsServer("396 " + user->mNickName + " " + user->mvHost + " :is now your hidden host");
					std::string modos = "+";
					if (user->getMode('r') == true)
						modos.append("r");
					if (user->getMode('z') == true)
						modos.append("z");
					if (user->getMode('w') == true)
						modos.append("w");
					if (user->getMode('o') == true)
						modos.append("o");
					std::string identi = "ZeusiRCd";
					if (!user->mIdent.empty())
						identi = user->mIdent;
					else
						user->mIdent = identi;
					Server::Send("SNICK " + user->mNickName + " " + identi + " " + user->mHost + " " + user->mCloak + " " + user->mvHost + " " + std::to_string(user->bLogin) + " " + user->mServer + " " + modos);
					if (user->getMode('r') == true)
						NickServ::checkmemos(user);
				}
			}
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Nick);
}
