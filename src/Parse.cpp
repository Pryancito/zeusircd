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

#include "ZeusBaseClass.h"
#include "Config.h"
#include "Utils.h"
#include "services.h"
#include "oper.h"
#include "sha256.h"
#include "Server.h"
#include "Channel.h"
#include "mainframe.h"
#include "db.h"
#include "services.h"
#include "module.h"

#include <string>
#include <iterator>
#include <algorithm>
#include <mutex>
#include <fstream>

extern time_t encendido;
extern OperSet miRCOps;
extern Memos MemoMsg;
std::map<std::string, unsigned int> bForce;
extern std::set <Server*> Servers;
extern std::mutex log_mtx;

std::string& ltrim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
{
    str.erase(0, str.find_first_not_of(chars));
    return str;
}
 
std::string& rtrim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
{
    str.erase(str.find_last_not_of(chars) + 1);
    return str;
}
 
std::string& trim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
{
    return ltrim(rtrim(str, chars), chars);
}

void User::log(const std::string &message) {
	log_mtx.lock();
	Channel *chan = Mainframe::instance()->getChannelByName("#debug");
	if (config["hub"].as<std::string>() == config["serverName"].as<std::string>()) {
		time_t now = time(0);
		struct tm tm;
		localtime_r(&now, &tm);
		char date[32];
		strftime(date, sizeof(date), "%r %d-%m-%Y", &tm);
		std::ofstream fileLog("ircd.log", std::ios::out | std::ios::app);
		if (fileLog.fail()) {
			if (chan)
				chan->broadcast(":" + config["operserv"].as<std::string>() + " PRIVMSG #debug :Error opening log file.");
			log_mtx.unlock();
			return;
		}
		fileLog << date << " -> " << message << std::endl;
		fileLog.close();
	}
	if (chan) {
		chan->broadcast(":" + config["operserv"].as<std::string>() + " PRIVMSG #debug :" + message);
		Server::Send("PRIVMSG " + config["operserv"].as<std::string>() + " #debug " + message);
	}
	log_mtx.unlock();
}

void LocalUser::Parse(std::string message)
{
	if (quit == true)
		return;
	trim(message);
	std::vector<std::string> results;
	
	Utils::split(message, results, " ");
	if (results.size() == 0)
		return;
	
	std::string cmd = results[0];
	std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
	
	if (cmd == "RELOAD")
	{
		if (getMode('o') == false) {
			SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You do not have iRCop privileges."));
			return;
		} else {
			int unloaded = Module::UnloadAll();
			int loaded = Module::LoadAll();
			if (loaded == -1 || unloaded == -1) {
				SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "Problem loading modules. ircd stopped"));
				exit(1);
			} else
				SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "Unloaded %d modules. Loaded %d modules.", unloaded, loaded));
			return;
		}
	}
	else if (cmd == "REHASH")
	{
		if (getMode('o') == false) {
			SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "You do not have iRCop privileges."));
			return;
		} else {
			config = YAML::LoadFile(config_file);
			SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "The config has been reloaded."));
			return;
		}
	}
	
	bool executed = false;
	std::vector<Widget *> comm;
	for (Widget* w : commands) {
		if (w == nullptr) continue;
		else if (w->cmd == cmd) {
			w->command(this, message);
			executed = true;
			if (w->must_end == true)
				return;
		}
	}
	if (!executed)
		SendAsServer("421 " + mNickName + " :" + Utils::make_string(mLang, "Unknown command.") + " ( " + cmd + " )");
/*
	switch (str2int(cmd.c_str()))
	{
		case str2int("RELEASE"): do_cmd_release(message); break;
		case str2int("TOPIC"): do_cmd_topic(message); break;
		case str2int("LIST"): do_cmd_list(message); break;
		case str2int("KICK"): do_cmd_kick(message); break;
		case str2int("NAMES"): do_cmd_names(message); break;
		case str2int("WHO"): do_cmd_who(message); break;
		case str2int("AWAY"): do_cmd_away(message); break;
		case str2int("OPER"): do_cmd_oper(message); break;
		case str2int("USERHOST"): do_cmd_userhost(); break;
		case str2int("WEBIRC"): do_cmd_webirc(message); break;
		case str2int("STATS"): do_cmd_stats(); break;
		case str2int("LUSERS"): do_cmd_stats(); break;
		case str2int("OPERS"): do_cmd_opers(); break;
		case str2int("UPTIME"): do_cmd_uptime(); break;
		case str2int("VERSION"): do_cmd_version(); break;
		case str2int("MODE"): do_cmd_mode(message); break;
		case str2int("WHOIS"): do_cmd_whois(message); break;
		case str2int("SERVERS"): do_cmd_servers(); break;
		case str2int("CONNECT"): do_cmd_connect(message); break;
		case str2int("SQUIT"): do_cmd_squit(message); break;
		default: SendAsServer("421 " + mNickName + " :" + Utils::make_string(mLang, "Unknown command.")); break;
	}*/
}
/*
void LocalUser::do_cmd_release(std::string message) {
	std::vector<std::string> results;
	Utils::split(message, results, " ");
	if (!bSentNick) {
		SendAsServer("461 ZeusiRCd :" + Utils::make_string(mLang, "You havent used the NICK command yet, you have limited access."));
		return;
	} else if (results.size() < 2) {
		SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "More data is needed."));
		return;
	} else if (getMode('o') == false) {
		SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "You do not have iRCop privileges."));
		return;
	} else if (checknick(results[1]) == false) {
		SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "The nick contains no-valid characters."));
		return;
	} else if ((bForce.find(results[1])) != bForce.end()) {
		bForce.erase(results[1]);
		SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "The nick has been released."));
		return;
	} else {
		SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "The nick isn't in BruteForce lists."));
		return;
	}
}

void LocalUser::do_cmd_topic(std::string message) {
	std::vector<std::string> results;
	Utils::split(message, results, " ");
	if (!bSentNick) {
		SendAsServer("461 ZeusiRCd :" + Utils::make_string(mLang, "You havent used the NICK command yet, you have limited access."));
		return;
	}
	else if (results.size() == 2) {
		Channel* chan = Mainframe::instance()->getChannelByName(results[1]);
		if (chan) {
			if (chan->topic().empty()) {
				SendAsServer("331 " + chan->name() + " :" + Utils::make_string(mLang, "No topic is set !"));
			}
			else {
				SendAsServer("332 " + mNickName + " " + chan->name() + " :" + chan->topic());
			}
		}
	}
}

void LocalUser::do_cmd_list(std::string message) {
	std::vector<std::string> results;
	Utils::split(message, results, " ");
	if (!bSentNick) {
		SendAsServer("461 ZeusiRCd :" + Utils::make_string(mLang, "You havent used the NICK command yet, you have limited access."));
		return;
	}
	std::string comodin = "*";
	if (results.size() == 2)
		comodin = results[1];
	SendAsServer("321 " + mNickName + " " + Utils::make_string(mLang, "Channel :Users Name"));
	auto channels = Mainframe::instance()->channels();
	auto it = channels.begin();
	for (; it != channels.end(); ++it) {
		std::string mtch = it->second->name();
		std::transform(comodin.begin(), comodin.end(), comodin.begin(), ::tolower);
		std::transform(mtch.begin(), mtch.end(), mtch.begin(), ::tolower);
		if (Utils::Match(comodin.c_str(), mtch.c_str()) == 1)
			SendAsServer("322 " + mNickName + " " + it->second->name() + " " + std::to_string(it->second->userCount()) + " :" + it->second->topic());
	}
	SendAsServer("323 " + mNickName + " :" + Utils::make_string(mLang, "End of /LIST"));
}


void LocalUser::do_cmd_kick(std::string message) {
	std::vector<std::string> results;
	Utils::split(message, results, " ");
	if (mNickName == "") {
		SendAsServer("461 ZeusiRCd :" + Utils::make_string(mLang, "You havent used the NICK command yet, you have limited access."));
		return;
	} else if (results.size() < 3) return;
	Channel* chan = Mainframe::instance()->getChannelByName(results[1]);
	LocalUser*  victim = Mainframe::instance()->getLocalUserByName(results[2]);
	RemoteUser*  rvictim = Mainframe::instance()->getRemoteUserByName(results[2]);
	std::string reason = "";
	if (chan && victim) {
		for (unsigned int i = 3; i < results.size(); ++i) {
			reason += results[i] + " ";
		}
		trim(reason);
		if ((chan->isOperator(this) || chan->isHalfOperator(this)) && chan->hasUser(victim) && (!chan->isOperator(victim) || getMode('o') == true) && victim->getMode('o') == false) {
			victim->cmdKick(mNickName, victim->mNickName, reason, chan);
			Server::Send("SKICK " + mNickName + " " + chan->name() + " " + victim->mNickName + " " + reason);
		}
	} if (chan && rvictim) {
		for (unsigned int i = 3; i < results.size(); ++i) {
			reason += results[i] + " ";
		}
		trim(reason);
		if ((chan->isOperator(this) || chan->isHalfOperator(this)) && chan->hasUser(rvictim) && (!chan->isOperator(victim) || getMode('o') == true) && rvictim->getMode('o') == false) {
			rvictim->SKICK(mNickName, rvictim->mNickName, reason, chan);
			Server::Send("SKICK " + mNickName + " " + chan->name() + " " + rvictim->mNickName + " " + reason);
		}
	}
}
	
void LocalUser::do_cmd_names(std::string message) {
	std::vector<std::string> results;
	Utils::split(message, results, " ");
	if (!bSentNick) {
		SendAsServer("461 ZeusiRCd :" + Utils::make_string(mLang, "You havent used the NICK command yet, you have limited access."));
		return;
	}
	else if (results.size() < 2) return;
	Channel* chan = Mainframe::instance()->getChannelByName(results[1]);
	if (!chan) {
		SendAsServer("403 " + mNickName + " :" + Utils::make_string(mLang, "The channel doesnt exists."));
		return;
	} else if (!chan->hasUser(this)) {
		SendAsServer("441 " + mNickName + " :" + Utils::make_string(mLang, "You are not into the channel."));
		return;
	} else {
		chan->sendUserList(this);
		return;
	}
}
	
void LocalUser::do_cmd_who(std::string message) {
	std::vector<std::string> results;
	Utils::split(message, results, " ");
	if (!bSentNick) {
		SendAsServer("461 ZeusiRCd :" + Utils::make_string(mLang, "You havent used the NICK command yet, you have limited access."));
		return;
	} else if (results.size() < 2) return;
	Channel* chan = Mainframe::instance()->getChannelByName(results[1]);
	if (!chan) {
		SendAsServer("403 " + mNickName + " :" + Utils::make_string(mLang, "The channel doesnt exists."));
		return;
	} else if (!chan->hasUser(this)) {
		SendAsServer("441 " + mNickName + " :" + Utils::make_string(mLang, "You are not into the channel."));
		return;
	} else {
		chan->sendWhoList(this);
		return;
	}
}

void LocalUser::do_cmd_away(std::string message) {
	std::vector<std::string> results;
	Utils::split(message, results, " ");
	if (!bSentNick) {
		SendAsServer("461 ZeusiRCd :" + Utils::make_string(mLang, "You havent used the NICK command yet, you have limited access."));
		return;
	} else if (results.size() == 1) {
		cmdAway("", false);
		SendAsServer("305 " + mNickName + " :AWAY OFF");
		return;
	} else {
		std::string away = "";
		for (unsigned int i = 1; i < results.size(); ++i) { away.append(results[i] + " "); }
		trim(away);
		cmdAway(away, true);
		SendAsServer("306 " + mNickName + " :AWAY ON " + away);
		return;
	}
}

void LocalUser::do_cmd_oper(std::string message) {
	std::vector<std::string> results;
	Utils::split(message, results, " ");
	Oper oper;
	if (!bSentNick) {
		SendAsServer("461 ZeusiRCd :" + Utils::make_string(mLang, "You havent used the NICK command yet, you have limited access."));
		return;
	} else if (results.size() < 3) {
		SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "More data is needed."));
		return;
	} else if (getMode('o') == true) {
		SendAsServer("381 " + mNickName + " :" + Utils::make_string(mLang, "You are already an iRCop."));
	} else if (oper.Login(this, results[1], results[2]) == true) {
		SendAsServer("381 " + mNickName + " :" + Utils::make_string(mLang, "Now you are an iRCop."));
	} else {
		SendAsServer("481 " + mNickName + " :" + Utils::make_string(mLang, "Login failed, your attempt has been notified."));
	}
}

void LocalUser::do_cmd_userhost() {
	return;
}

void LocalUser::do_cmd_webirc(std::string message) {
	std::vector<std::string> results;
	Utils::split(message, results, " ");
	if (results.size() < 5) {
		SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "More data is needed."));
		return;
	} else if (results[1] == config["cgiirc"].as<std::string>()) {
		mHost = results[4];
		return;
	} else {
		Close();
	}
}

void LocalUser::do_cmd_stats() {
	SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "There are \002%s\002 users and \002%s\002 channels.", std::to_string(Mainframe::instance()->countusers()).c_str(), std::to_string(Mainframe::instance()->countchannels()).c_str()));
	SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "There are \002%s\002 registered nicks and \002%s\002 registered channels.", std::to_string(NickServ::GetNicks()).c_str(), std::to_string(ChanServ::GetChans()).c_str()));
	SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "There are \002%s\002 connected iRCops.", std::to_string(Oper::Count()).c_str()));
	SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "There are \002%s\002 connected servers.", std::to_string(Server::count()).c_str()));
}
	
void LocalUser::do_cmd_opers() {
	for (auto oper : miRCOps) {
		LocalUser *user = Mainframe::instance()->getLocalUserByName(oper);
		if (user) {
			std::string online = " ( \0033ONLINE\003 )";
			if (user->bAway)
				online = " ( \0034AWAY\003 )";
			SendAsServer("002 " + mNickName + " :" + user->mNickName + online);
		}
		RemoteUser *ruser = Mainframe::instance()->getRemoteUserByName(oper);
		if (ruser) {
			std::string online = " ( \0033ONLINE\003 )";
			if (ruser->bAway)
				online = " ( \0034AWAY\003 )";
			SendAsServer("002 " + mNickName + " :" + ruser->mNickName + online);
		}
	}
}
	
void LocalUser::do_cmd_uptime() {
	SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "This server started as long as: %s", Utils::Time(encendido).c_str()));
}

void LocalUser::do_cmd_version() {
	SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "Version of ZeusiRCd: %s", config["version"].as<std::string>().c_str()));
	if (getMode('o') == true)
		SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "Version of DataBase: %s", DB::GetLastRecord().c_str()));
}
	
void LocalUser::do_cmd_mode(std::string message) {
	std::vector<std::string> results;
	Utils::split(message, results, " ");
	if (!bSentNick) {
		SendAsServer("461 ZeusiRCd :" + Utils::make_string(mLang, "You havent used the NICK command yet, you have limited access."));
		return;
	}
	else if (results.size() < 2) {
		SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "More data is needed."));
		return;
	} else if (results[1][0] == '#') {
		if (checkchan(results[1]) == false) {
			SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The channel contains no-valid characters."));
			return;
		}
		Channel* chan = Mainframe::instance()->getChannelByName(results[1]);
		if (ChanServ::IsRegistered(results[1]) == false && getMode('o') == false) {
			SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The channel %s is not registered.", results[1].c_str()));
			return;
		} else if (!chan) {
			SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The channel is empty."));
			return;
		} else if (chan->isOperator(this) == false && chan->isHalfOperator(this) == false && results.size() != 2 && getMode('o') == false) {
			SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You do not have @ nor %."));
			return;
		} else if (results.size() == 2) {
			std::string sql = "SELECT MODOS from CANALES WHERE NOMBRE='" + results[1] + "';";
			std::string modos = DB::SQLiteReturnString(sql);
			SendAsServer("324 " + mNickName + " " + results[1] + " " + modos);
			return;
		} else if (results.size() == 3) {
			if (results[2] == "+b" || results[2] == "b") {
				for (auto ban : chan->bans())
					SendAsServer("367 " + mNickName + " " + results[1] + " " + ban->mask() + " " + ban->whois() + " " + std::to_string(ban->time()));
				SendAsServer("368 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "End of banned."));
			} else if (results[2] == "+B" || results[2] == "B") {
				for (auto ban : chan->pbans())
					SendAsServer("367 " + mNickName + " " + results[1] + " " + ban->mask() + " " + ban->whois() + " " + std::to_string(ban->time()));
				SendAsServer("368 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "End of banned."));
			}
		} else if (results.size() > 3) {
			bool action = 0;
			unsigned int j = 0;
			std::string ban;
			std::string msg = message.substr(5);
			for (unsigned int i = 0; i < results[2].length(); i++) {
				if (results.size()-3 == j)
					return;
				if (results[2][i] == '+') {
					action = 1;
				} else if (results[2][i] == '-') {
					action = 0;
				} else if (results[2][i] == 'b') {
					std::vector<std::string> baneos;
					std::string maskara = results[3+j];
					std::transform(maskara.begin(), maskara.end(), maskara.begin(), ::tolower);
					if (action == 1) {
						if (chan->IsBan(maskara) == true) {
							Send(":" + config["chanserv"].as<std::string>() + " NOTICE " + mNickName + " :" + Utils::make_string(mLang, "The BAN already exist."));
						} else {
							chan->setBan(results[3+j], mNickName);
							chan->broadcast(messageHeader() + "MODE " + chan->name() + " +b " + results[3+j]);
							Send(":" + config["chanserv"].as<std::string>() + " NOTICE " + mNickName + " :" + Utils::make_string(mLang, "The BAN has been set."));
							Server::Send("CMODE " + mNickName + " " + chan->name() + " +b " + results[3+j]);
						}
					} else {
						if (chan->IsBan(maskara) == false) {
							Send(":" + config["chanserv"].as<std::string>() + " NOTICE " + mNickName + " :" + Utils::make_string(mLang, "The BAN does not exist."));
						} else {
							BanSet bans = chan->bans();
							BanSet::iterator it = bans.begin();
							for (; it != bans.end(); ++it)
								if ((*it)->mask() == results[3+j]) {
									chan->UnBan((*it));
									chan->broadcast(messageHeader() + "MODE " + chan->name() + " -b " + results[3+j]);
									Send(":" + config["chanserv"].as<std::string>() + " NOTICE " + mNickName + " :" + Utils::make_string(mLang, "The BAN has been deleted."));
									Server::Send("CMODE " + mNickName + " " + chan->name() + " -b " + results[3+j]);
								}
						}
					}
					j++;
				} else if (results[2][i] == 'B') {
					std::vector<std::string> baneos;
					std::string maskara = results[3+j];
					std::transform(maskara.begin(), maskara.end(), maskara.begin(), ::tolower);
					if (action == 1) {
						if (chan->IsBan(maskara) == true) {
							Send(":" + config["chanserv"].as<std::string>() + " NOTICE " + mNickName + " :" + Utils::make_string(mLang, "The BAN already exist."));
						} else {
							chan->setpBan(results[3+j], mNickName);
							chan->broadcast(messageHeader() + "MODE " + chan->name() + " +B " + results[3+j]);
							Send(":" + config["chanserv"].as<std::string>() + " NOTICE " + mNickName + " :" + Utils::make_string(mLang, "The BAN has been set."));
							Server::Send("CMODE " + mNickName + " " + chan->name() + " +B " + results[3+j]);
						}
					} else {
						if (chan->IsBan(maskara) == false) {
							Send(":" + config["chanserv"].as<std::string>() + " NOTICE " + mNickName + " :" + Utils::make_string(mLang, "The BAN does not exist."));
						} else {
							pBanSet bans = chan->pbans();
							pBanSet::iterator it = bans.begin();
							for (; it != bans.end(); ++it)
								if ((*it)->mask() == results[3+j]) {
									chan->UnpBan((*it));
									chan->broadcast(messageHeader() + "MODE " + chan->name() + " -B " + results[3+j]);
									Send(":" + config["chanserv"].as<std::string>() + " NOTICE " + mNickName + " :" + Utils::make_string(mLang, "The BAN has been deleted."));
									Server::Send("CMODE " + mNickName + " " + chan->name() + " -B " + results[3+j]);
								}
						}
					}
					j++;
				} else if (results[2][i] == 'o') {
					{
						LocalUser *target = Mainframe::instance()->getLocalUserByName(results[3+j]);
						if (target)
						{
							if (chan->hasUser(target) == false) { j++; continue; }
							if (action == 1) {
								if (getMode('o') == true && (chan->isOperator(target) == false)) {
									if (chan->isVoice(target) == true) {
										chan->broadcast(messageHeader() + "MODE " + chan->name() + " -v " + target->mNickName);
										Server::Send("CMODE " + mNickName + " " + chan->name() + " -v " + target->mNickName);
										chan->delVoice(target);
									} if (chan->isHalfOperator(target) == true) {
										chan->broadcast(messageHeader() + "MODE " + chan->name() + " -h " + target->mNickName);
										Server::Send("CMODE " + mNickName + " " + chan->name() + " -h " + target->mNickName);
										chan->delHalfOperator(target);
									}
									chan->broadcast(messageHeader() + "MODE " + chan->name() + " +o " + target->mNickName);
									Server::Send("CMODE " + mNickName + " " + chan->name() + " +o " + target->mNickName);
									chan->giveOperator(target);
								} else if (chan->isOperator(target) == true) { j++; continue; }
								else if (chan->isOperator(this) == false) { j++; continue; }
								else {
									if (chan->isVoice(target) == true) {
										chan->broadcast(messageHeader() + "MODE " + chan->name() + " -v " + target->mNickName);
										Server::Send("CMODE " + mNickName + " " + chan->name() + " -v " + target->mNickName);
										chan->delVoice(target);
									} if (chan->isHalfOperator(target) == true) {
										chan->broadcast(messageHeader() + "MODE " + chan->name() + " -h " + target->mNickName);
										Server::Send("CMODE " + mNickName + " " + chan->name() + " -h " + target->mNickName);
										chan->delHalfOperator(target);
									}
									chan->broadcast(messageHeader() + "MODE " + chan->name() + " +o " + target->mNickName);
									Server::Send("CMODE " + mNickName + " " + chan->name() + " +o " + target->mNickName);
									chan->giveOperator(target);
								}
							} else {
								if (chan->isOperator(this) == true && chan->isOperator(target) == true) {
									chan->broadcast(messageHeader() + "MODE " + chan->name() + " -o " + target->mNickName);
									Server::Send("CMODE " + mNickName + " " + chan->name() + " -o " + target->mNickName);
									chan->delOperator(target);
								}
							}
						}
					}
					{
						RemoteUser *target = Mainframe::instance()->getRemoteUserByName(results[3+j]);
						if (target)
						{
							if (chan->hasUser(target) == false) { j++; continue; }
							if (action == 1) {
								if (getMode('o') == true && (chan->isOperator(target) == false)) {
									if (chan->isVoice(target) == true) {
										chan->broadcast(messageHeader() + "MODE " + chan->name() + " -v " + target->mNickName);
										Server::Send("CMODE " + mNickName + " " + chan->name() + " -v " + target->mNickName);
										chan->delVoice(target);
									} if (chan->isHalfOperator(target) == true) {
										chan->broadcast(messageHeader() + "MODE " + chan->name() + " -h " + target->mNickName);
										Server::Send("CMODE " + mNickName + " " + chan->name() + " -h " + target->mNickName);
										chan->delHalfOperator(target);
									}
									chan->broadcast(messageHeader() + "MODE " + chan->name() + " +o " + target->mNickName);
									Server::Send("CMODE " + mNickName + " " + chan->name() + " +o " + target->mNickName);
									chan->giveOperator(target);
								} else if (chan->isOperator(target) == true) { j++; continue; }
								else if (chan->isOperator(this) == false) { j++; continue; }
								else {
									if (chan->isVoice(target) == true) {
										chan->broadcast(messageHeader() + "MODE " + chan->name() + " -v " + target->mNickName);
										Server::Send("CMODE " + mNickName + " " + chan->name() + " -v " + target->mNickName);
										chan->delVoice(target);
									} if (chan->isHalfOperator(target) == true) {
										chan->broadcast(messageHeader() + "MODE " + chan->name() + " -h " + target->mNickName);
										Server::Send("CMODE " + mNickName + " " + chan->name() + " -h " + target->mNickName);
										chan->delHalfOperator(target);
									}
									chan->broadcast(messageHeader() + "MODE " + chan->name() + " +o " + target->mNickName);
									Server::Send("CMODE " + mNickName + " " + chan->name() + " +o " + target->mNickName);
									chan->giveOperator(target);
								}
							} else {
								if (chan->isOperator(this) == true && chan->isOperator(target) == true) {
									chan->broadcast(messageHeader() + "MODE " + chan->name() + " -o " + target->mNickName);
									Server::Send("CMODE " + mNickName + " " + chan->name() + " -o " + target->mNickName);
									chan->delOperator(target);
								}
							}
						}
					}
					j++;
				} else if (results[2][i] == 'h') {
					{
						LocalUser *target = Mainframe::instance()->getLocalUserByName(results[3+j]);
						if (target)
						{
							if (chan->hasUser(target) == false) { j++; continue; }
							if (action == 1) {
								if (getMode('o') == true && chan->isHalfOperator(target) == false) {
									if (chan->isOperator(target) == true) {
										chan->broadcast(messageHeader() + "MODE " + chan->name() + " -o " + target->mNickName);
										Server::Send("CMODE " + mNickName + " " + chan->name() + " -o " + target->mNickName);
										chan->delOperator(target);
									} if (chan->isVoice(target) == true) {
										chan->broadcast(messageHeader() + "MODE " + chan->name() + " -v " + target->mNickName);
										Server::Send("CMODE " + mNickName + " " + chan->name() + " -v " + target->mNickName);
										chan->delVoice(target);
									}
									chan->broadcast(messageHeader() + "MODE " + chan->name() + " +h " + target->mNickName);
									Server::Send("CMODE " + mNickName + " " + chan->name() + " +h " + target->mNickName);
									chan->giveHalfOperator(target);
								} else if (chan->isHalfOperator(target) == true) { j++; continue; }
								else if (chan->isOperator(this) == false) { j++; continue; }
								else {
									if (chan->isOperator(target) == true) {
										chan->broadcast(messageHeader() + "MODE " + chan->name() + " -o " + target->mNickName);
										Server::Send("CMODE " + mNickName + " " + chan->name() + " -o " + target->mNickName);
										chan->delOperator(target);
									} if (chan->isVoice(target) == true) {
										chan->broadcast(messageHeader() + "MODE " + chan->name() + " -v " + target->mNickName);
										Server::Send("CMODE " + mNickName + " " + chan->name() + " -v " + target->mNickName);
										chan->delVoice(target);
									}
									chan->broadcast(messageHeader() + "MODE " + chan->name() + " +h " + target->mNickName);
									Server::Send("CMODE " + mNickName + " " + chan->name() + " +h " + target->mNickName);
									chan->giveHalfOperator(target);
								}
							} else {
								if (chan->isHalfOperator(target) == true || chan->isOperator(this) == true) {
									chan->broadcast(messageHeader() + "MODE " + chan->name() + " -h " + target->mNickName);
									Server::Send("CMODE " + mNickName + " " + chan->name() + " -h " + target->mNickName);
									chan->delHalfOperator(target);
								}
							}
						}
					}
					{
						RemoteUser *target = Mainframe::instance()->getRemoteUserByName(results[3+j]);
						if (target)
						{
							if (chan->hasUser(target) == false) { j++; continue; }
							if (action == 1) {
								if (getMode('o') == true && chan->isHalfOperator(target) == false) {
									if (chan->isOperator(target) == true) {
										chan->broadcast(messageHeader() + "MODE " + chan->name() + " -o " + target->mNickName);
										Server::Send("CMODE " + mNickName + " " + chan->name() + " -o " + target->mNickName);
										chan->delOperator(target);
									} if (chan->isVoice(target) == true) {
										chan->broadcast(messageHeader() + "MODE " + chan->name() + " -v " + target->mNickName);
										Server::Send("CMODE " + mNickName + " " + chan->name() + " -v " + target->mNickName);
										chan->delVoice(target);
									}
									chan->broadcast(messageHeader() + "MODE " + chan->name() + " +h " + target->mNickName);
									Server::Send("CMODE " + mNickName + " " + chan->name() + " +h " + target->mNickName);
									chan->giveHalfOperator(target);
								} else if (chan->isHalfOperator(target) == true) { j++; continue; }
								else if (chan->isOperator(this) == false) { j++; continue; }
								else {
									if (chan->isOperator(target) == true) {
										chan->broadcast(messageHeader() + "MODE " + chan->name() + " -o " + target->mNickName);
										Server::Send("CMODE " + mNickName + " " + chan->name() + " -o " + target->mNickName);
										chan->delOperator(target);
									} if (chan->isVoice(target) == true) {
										chan->broadcast(messageHeader() + "MODE " + chan->name() + " -v " + target->mNickName);
										Server::Send("CMODE " + mNickName + " " + chan->name() + " -v " + target->mNickName);
										chan->delVoice(target);
									}
									chan->broadcast(messageHeader() + "MODE " + chan->name() + " +h " + target->mNickName);
									Server::Send("CMODE " + mNickName + " " + chan->name() + " +h " + target->mNickName);
									chan->giveHalfOperator(target);
								}
							} else {
								if (chan->isHalfOperator(target) == true || chan->isOperator(this) == true) {
									chan->broadcast(messageHeader() + "MODE " + chan->name() + " -h " + target->mNickName);
									Server::Send("CMODE " + mNickName + " " + chan->name() + " -h " + target->mNickName);
									chan->delHalfOperator(target);
								}
							}
						}
					}
					j++;
				} else if (results[2][i] == 'v') {
					{
						LocalUser *target = Mainframe::instance()->getLocalUserByName(results[3+j]);
						if (target)
						{
							if (chan->hasUser(target) == false) { j++; continue; }
							if (action == 1) {
								if (getMode('o') == true && chan->isVoice(target) == false) {
									if (chan->isOperator(target) == true) {
										chan->broadcast(messageHeader() + "MODE " + chan->name() + " -o " + target->mNickName);
										Server::Send("CMODE " + mNickName + " " + chan->name() + " -o " + target->mNickName);
										chan->delOperator(target);
									} if (chan->isHalfOperator(target) == true) {
										chan->broadcast(messageHeader() + "MODE " + chan->name() + " -h " + target->mNickName);
										Server::Send("CMODE " + mNickName + " " + chan->name() + " -h " + target->mNickName);
										chan->delHalfOperator(target);
									}
									chan->broadcast(messageHeader() + "MODE " + chan->name() + " +v " + target->mNickName);
									Server::Send("CMODE " + mNickName + " " + chan->name() + " +v " + target->mNickName);
									chan->giveVoice(target);
								} else if (chan->isVoice(target) == true) { j++; continue; }
								else if (chan->isOperator(this) == false && chan->isHalfOperator(this) == false) { j++; continue; }
								else {
									if (chan->isOperator(target) == true) {
										chan->broadcast(messageHeader() + "MODE " + chan->name() + " -o " + target->mNickName);
										Server::Send("CMODE " + mNickName + " " + chan->name() + " -o " + target->mNickName);
										chan->delOperator(target);
									} if (chan->isHalfOperator(target) == true) {
										chan->broadcast(messageHeader() + "MODE " + chan->name() + " -h " + target->mNickName);
										Server::Send("CMODE " + mNickName + " " + chan->name() + " -h " + target->mNickName);
										chan->delHalfOperator(target);
									}
									chan->broadcast(messageHeader() + "MODE " + chan->name() + " +v " + target->mNickName);
									Server::Send("CMODE " + mNickName + " " + chan->name() + " +v " + target->mNickName);
									chan->giveVoice(target);
								}
							} else {
								if (chan->isVoice(target) == true && (chan->isOperator(this) == true || chan->isHalfOperator(this) == true)) {
									chan->broadcast(messageHeader() + "MODE " + chan->name() + " -v " + target->mNickName);
									Server::Send("CMODE " + mNickName + " " + chan->name() + " -v " + target->mNickName);
									chan->delVoice(target);
								}
							}
						}
					}
					{
						RemoteUser *target = Mainframe::instance()->getRemoteUserByName(results[3+j]);
						if (target)
						{
							if (chan->hasUser(target) == false) { j++; continue; }
							if (action == 1) {
								if (getMode('o') == true && chan->isVoice(target) == false) {
									if (chan->isOperator(target) == true) {
										chan->broadcast(messageHeader() + "MODE " + chan->name() + " -o " + target->mNickName);
										Server::Send("CMODE " + mNickName + " " + chan->name() + " -o " + target->mNickName);
										chan->delOperator(target);
									} if (chan->isHalfOperator(target) == true) {
										chan->broadcast(messageHeader() + "MODE " + chan->name() + " -h " + target->mNickName);
										Server::Send("CMODE " + mNickName + " " + chan->name() + " -h " + target->mNickName);
										chan->delHalfOperator(target);
									}
									chan->broadcast(messageHeader() + "MODE " + chan->name() + " +v " + target->mNickName);
									Server::Send("CMODE " + mNickName + " " + chan->name() + " +v " + target->mNickName);
									chan->giveVoice(target);
								} else if (chan->isVoice(target) == true) { j++; continue; }
								else if (chan->isOperator(this) == false && chan->isHalfOperator(this) == false) { j++; continue; }
								else {
									if (chan->isOperator(target) == true) {
										chan->broadcast(messageHeader() + "MODE " + chan->name() + " -o " + target->mNickName);
										Server::Send("CMODE " + mNickName + " " + chan->name() + " -o " + target->mNickName);
										chan->delOperator(target);
									} if (chan->isHalfOperator(target) == true) {
										chan->broadcast(messageHeader() + "MODE " + chan->name() + " -h " + target->mNickName);
										Server::Send("CMODE " + mNickName + " " + chan->name() + " -h " + target->mNickName);
										chan->delHalfOperator(target);
									}
									chan->broadcast(messageHeader() + "MODE " + chan->name() + " +v " + target->mNickName);
									Server::Send("CMODE " + mNickName + " " + chan->name() + " +v " + target->mNickName);
									chan->giveVoice(target);
								}
							} else {
								if (chan->isVoice(target) == true && (chan->isOperator(this) == true || chan->isHalfOperator(this) == true)) {
									chan->broadcast(messageHeader() + "MODE " + chan->name() + " -v " + target->mNickName);
									Server::Send("CMODE " + mNickName + " " + chan->name() + " -v " + target->mNickName);
									chan->delVoice(target);
								}
							}
						}
					}
					j++;
				}
			}
		}
	}
}
	
void LocalUser::do_cmd_whois(std::string message) {
	std::vector<std::string> results;
	Utils::split(message, results, " ");
	if (!bSentNick) {
		SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You havent used the NICK command yet, you have limited access."));
		return;
	} else if (results.size() < 2) {
		SendAsServer("431 " + mNickName + " :" + Utils::make_string(mLang, "More data is needed."));
		return;
	} else if (results[1][0] == '#') {
		if (checkchan (results[1]) == false) {
			SendAsServer("401 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "The channel contains no-valid characters."));
			SendAsServer("318 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "End of /WHOIS."));
			return;
		}
		Channel* chan = Mainframe::instance()->getChannelByName(results[1]);
		std::string sql;
		std::string mascara = mNickName + "!" + mIdent + "@" + mCloak;
		if (ChanServ::IsAKICK(mascara, results[1]) == true) {
			SendAsServer("320 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "STATUS: \0036AKICK\003."));
		} else if (chan && chan->IsBan(mascara) == true)
			SendAsServer("320 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "STATUS: \0036BANNED\003."));
		if (getMode('r') == true && !NickServ::GetvHost(mNickName).empty()) {
			mascara = mNickName + "!" + mIdent + "@" + mvHost;
			if (ChanServ::IsAKICK(mascara, results[1]) == true) {
				SendAsServer("320 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "STATUS: \0036AKICK\003."));
			} else if (chan && chan->IsBan(mascara) == true)
				SendAsServer("320 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "STATUS: \0036BANNED\003."));
		} if (!chan)
			SendAsServer("320 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "STATUS: \0034EMPTY\003."));
		else
			SendAsServer("320 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "STATUS: \0033ACTIVE\003."));
		if (ChanServ::IsRegistered(results[1]) == 1) {
			SendAsServer("320 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "The channel is registered."));
			switch (ChanServ::Access(mNickName, results[1])) {
				case 0:
					SendAsServer("320 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "You do not have access."));
					break;
				case 1:
					SendAsServer("320 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "Your access is VOP."));
					break;
				case 2:
					SendAsServer("320 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "Your access is HOP."));
					break;
				case 3:
					SendAsServer("320 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "Your access is AOP."));
					break;
				case 4:
					SendAsServer("320 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "Your access is SOP."));
					break;
				case 5:
					SendAsServer("320 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "Your access is FOUNDER."));
					break;
				default:
					break;
			}
			std::string modes;
			if (ChanServ::HasMode(results[1], "FLOOD") > 0) {
				modes.append(" flood");
			} if (ChanServ::HasMode(results[1], "ONLYREG") == true) {
				modes.append(" onlyreg");
			} if (ChanServ::HasMode(results[1], "AUTOVOICE") == true) {
				modes.append(" autovoice");
			} if (ChanServ::HasMode(results[1], "MODERATED") == true) {
				modes.append(" moderated");
			} if (ChanServ::HasMode(results[1], "ONLYSECURE") == true) {
				modes.append(" onlysecure");
			} if (ChanServ::HasMode(results[1], "NONICKCHANGE") == true) {
				modes.append(" nonickchange");
			} if (ChanServ::HasMode(results[1], "COUNTRY") == true) {
				modes.append(" country");
			} if (ChanServ::HasMode(results[1], "ONLYACCESS") == true) {
				modes.append(" onlyaccess");
			}
			if (modes.empty() == true)
				SendAsServer("320 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "The channel has no modes."));
			else
				SendAsServer("320 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "The channel has modes:%s", modes.c_str()));
			if (ChanServ::IsKEY(results[1]) == 1 && ChanServ::Access(mNickName, results[1]) != 0) {
				sql = "SELECT CLAVE FROM CANALES WHERE NOMBRE='" + results[1] + "';";
				std::string key = DB::SQLiteReturnString(sql);
				if (key.length() > 0)
					SendAsServer("320 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "The channel key is: %s", key.c_str()));
			}
			std::string sql = "SELECT OWNER FROM CANALES WHERE NOMBRE='" + results[1] + "';";
			std::string owner = DB::SQLiteReturnString(sql);
			if (owner.length() > 0)
				SendAsServer("320 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "The founder of the channel is: %s", owner.c_str()));
			sql = "SELECT REGISTERED FROM CANALES WHERE NOMBRE='" + results[1] + "';";
			int registro = DB::SQLiteReturnInt(sql);
			if (registro > 0) {
				std::string tiempo = Utils::Time(registro);
				SendAsServer("320 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "Registered for: %s", tiempo.c_str()));
			}
			SendAsServer("318 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "End of /WHOIS."));
		} else {
			SendAsServer("401 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "The channel is not registered."));
			SendAsServer("318 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "End of /WHOIS."));
		}
	} else {
		if (checknick (results[1]) == false) {
			SendAsServer("401 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "The nick contains no-valid characters."));
			SendAsServer("318 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "End of /WHOIS."));
			return;
		}
		User *target = Mainframe::instance()->getUserByName(results[1]);
		std::string sql;
		if (!target && NickServ::IsRegistered(results[1]) == true) {
			SendAsServer("320 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "STATUS: \0034OFFLINE\003."));
			SendAsServer("320 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "The nick is registered."));
			if (OperServ::IsOper(results[1]) == true)
				SendAsServer("320 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "Is an iRCop."));
			sql = "SELECT SHOWMAIL FROM OPTIONS WHERE NICKNAME='" + results[1] + "';";
			if (DB::SQLiteReturnInt(sql) == 1 || getMode('o') == true) {
				sql = "SELECT EMAIL FROM NICKS WHERE NICKNAME='" + results[1] + "';";
				std::string email = DB::SQLiteReturnString(sql);
				if (email.length() > 0)
				SendAsServer("320 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "The email is: %s", email.c_str()));
			}
			sql = "SELECT URL FROM NICKS WHERE NICKNAME='" + results[1] + "';";
			std::string url = DB::SQLiteReturnString(sql);
			if (url.length() > 0)
				SendAsServer("320 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "The website is: %s", url.c_str()));
			sql = "SELECT VHOST FROM NICKS WHERE NICKNAME='" + results[1] + "';";
			std::string vHost = DB::SQLiteReturnString(sql);
			if (vHost.length() > 0)
				SendAsServer("320 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "The vHost is: %s", vHost.c_str()));
			sql = "SELECT REGISTERED FROM NICKS WHERE NICKNAME='" + results[1] + "';";
			int registro = DB::SQLiteReturnInt(sql);
			if (registro > 0) {
				std::string tiempo = Utils::Time(registro);
				SendAsServer("320 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "Registered for: %s", tiempo.c_str()));
			}
			sql = "SELECT LASTUSED FROM NICKS WHERE NICKNAME='" + results[1] + "';";
			int last = DB::SQLiteReturnInt(sql);
			std::string tiempo = Utils::Time(last);
			if (tiempo.length() > 0 && last > 0)
				SendAsServer("320 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "Last seen on: %s", tiempo.c_str()));
			SendAsServer("318 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "End of /WHOIS."));
			return;
		} else if (target && NickServ::IsRegistered(results[1]) == 1) {
			SendAsServer("320 " + mNickName + " " + target->mNickName + " :" + Utils::make_string(mLang, "%s is: %s!%s@%s", target->mNickName.c_str(), target->mNickName.c_str(), target->mIdent.c_str(), target->mCloak.c_str()));
			SendAsServer("320 " + mNickName + " " + target->mNickName + " :" + Utils::make_string(mLang, "STATUS: \0033CONNECTED\003."));
			SendAsServer("320 " + mNickName + " " + target->mNickName + " :" + Utils::make_string(mLang, "The nick is registered."));
			if (getMode('o') == true || target->mNickName == mNickName) {
				SendAsServer("320 " + mNickName + " " + target->mNickName + " :" + Utils::make_string(mLang, "The IP is: %s", target->mHost.c_str()));
				SendAsServer("320 " + mNickName + " " + target->mNickName + " :" + Utils::make_string(mLang, "The server is: %s", target->mServer.c_str()));
			}
			if (target->getMode('o') == true)
				SendAsServer("320 " + mNickName + " " + target->mNickName + " :" + Utils::make_string(mLang, "Is an iRCop."));
			if (target->getMode('z') == true)
				SendAsServer("320 " + mNickName + " " + target->mNickName + " :" + Utils::make_string(mLang, "Connects trough a secure channel SSL."));
			if (target->getMode('w') == true)
				SendAsServer("320 " + mNickName + " " + target->mNickName + " :" + Utils::make_string(mLang, "Connects trough WebChat."));
			if (NickServ::GetOption("SHOWMAIL", target->mNickName) == true || getMode('o') == true) {
				sql = "SELECT EMAIL FROM NICKS WHERE NICKNAME='" + target->mNickName + "';";
				std::string email = DB::SQLiteReturnString(sql);
				if (email.length() > 0)
				SendAsServer("320 " + mNickName + " " + target->mNickName + " :" + Utils::make_string(mLang, "The email is: %s", email.c_str()));
			}
			sql = "SELECT URL FROM NICKS WHERE NICKNAME='" + target->mNickName + "';";
			std::string url = DB::SQLiteReturnString(sql);
			if (url.length() > 0)
				SendAsServer("320 " + mNickName + " " + target->mNickName + " :" + Utils::make_string(mLang, "The website is: %s", url.c_str()));
			sql = "SELECT VHOST FROM NICKS WHERE NICKNAME='" + target->mNickName + "';";
			std::string vHost = DB::SQLiteReturnString(sql);
			if (vHost.length() > 0)
				SendAsServer("320 " + mNickName + " " + target->mNickName + " :" + Utils::make_string(mLang, "The vHost is: %s", vHost.c_str()));
			SendAsServer("320 " + mNickName + " " + target->mNickName + " :" + Utils::make_string(mLang, "Country: %s", Utils::GetEmoji(target->mHost).c_str()));
			if (target->bAway == true)
				SendAsServer("320 " + mNickName + " " + target->mNickName + " :AWAY " + target->mAway);
			if (mNickName == target->mNickName && getMode('r') == true) {
				std::string opciones;
				if (NickServ::GetOption("NOACCESS", mNickName) == 1) {
					if (!opciones.empty())
						opciones.append(", ");
					opciones.append("NOACCESS");
				}
				if (NickServ::GetOption("SHOWMAIL", mNickName) == 1) {
					if (!opciones.empty())
						opciones.append(", ");
					opciones.append("SHOWMAIL");
				}
				if (NickServ::GetOption("NOMEMO", mNickName) == 1) {
					if (!opciones.empty())
						opciones.append(", ");
					opciones.append("NOMEMO");
				}
				if (NickServ::GetOption("NOOP", mNickName) == 1) {
					if (!opciones.empty())
						opciones.append(", ");
					opciones.append("NOOP");
				}
				if (NickServ::GetOption("ONLYREG", mNickName) == 1) {
					if (!opciones.empty())
						opciones.append(", ");
					opciones.append("ONLYREG");
				}
				if (NickServ::GetOption("NOCOLOR", mNickName) == 1) {
					if (!opciones.empty())
						opciones.append(", ");
					opciones.append("NOCOLOR");
				}
				if (opciones.length() == 0)
					opciones = Utils::make_string(mLang, "None");
				SendAsServer("320 " + mNickName + " " + target->mNickName + " :" + Utils::make_string(mLang, "Your options are: %s", opciones.c_str()));
			}
			sql = "SELECT REGISTERED FROM NICKS WHERE NICKNAME='" + target->mNickName + "';";
			int registro = DB::SQLiteReturnInt(sql);
			if (registro > 0) {
				std::string tiempo = Utils::Time(registro);
				SendAsServer("320 " + mNickName + " " + target->mNickName + " :" + Utils::make_string(mLang, "Registered for: %s", tiempo.c_str()));
			}
			std::string tiempo = Utils::Time(target->bLogin);
			if (tiempo.length() > 0)
				SendAsServer("320 " + mNickName + " " + target->mNickName + " :" + Utils::make_string(mLang, "Connected from: %s", tiempo.c_str()));
			SendAsServer("318 " + mNickName + " " + target->mNickName + " :" + Utils::make_string(mLang, "End of /WHOIS."));
			return;
		} else if (target && NickServ::IsRegistered(results[1]) == 0) {
			SendAsServer("320 " + mNickName + " " + target->mNickName + " :" + Utils::make_string(mLang, "%s is: %s!%s@%s", target->mNickName.c_str(), target->mNickName.c_str(), target->mIdent.c_str(), target->mCloak.c_str()));
			SendAsServer("320 " + mNickName + " " + target->mNickName + " :" + Utils::make_string(mLang, "STATUS: \0033CONNECTED\003."));
			if (getMode('o') == true) {
				SendAsServer("320 " + mNickName + " " + target->mNickName + " :" + Utils::make_string(mLang, "The IP is: %s", target->mHost.c_str()));
				SendAsServer("320 " + mNickName + " " + target->mNickName + " :" + Utils::make_string(mLang, "The server is: %s", target->mServer.c_str()));
			}
			if (target->getMode('o') == true)
				SendAsServer("320 " + mNickName + " " + target->mNickName + " :" + Utils::make_string(mLang, "Is an iRCop."));
			if (target->getMode('z') == true)
				SendAsServer("320 " + mNickName + " " + target->mNickName + " :" + Utils::make_string(mLang, "Connects trough a secure channel SSL."));
			if (target->getMode('w') == true)
				SendAsServer("320 " + mNickName + " " + target->mNickName + " :" + Utils::make_string(mLang, "Connects trough WebChat."));
			SendAsServer("320 " + mNickName + " " + target->mNickName + " :" + Utils::make_string(mLang, "Country: %s", Utils::GetEmoji(target->mHost).c_str()));
			if (target->bAway == true)
				SendAsServer("320 " + mNickName + " " + target->mNickName + " :AWAY " + target->mAway);
			std::string tiempo = Utils::Time(target->bLogin);
			if (tiempo.length() > 0)
				SendAsServer("320 " + mNickName + " " + target->mNickName + " :" + Utils::make_string(mLang, "Connected from: %s", tiempo.c_str()));
			SendAsServer("318 " + mNickName + " " + target->mNickName + " :" + Utils::make_string(mLang, "End of /WHOIS."));
			return;
		} else {
			SendAsServer("401 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "The nick does not exist."));
			SendAsServer("318 " + mNickName + " " + results[1] + " :" + Utils::make_string(mLang, "End of /WHOIS."));
			return;
		}
	}
}

void LocalUser::do_cmd_servers() {
	if (getMode('o') == false) {
		SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You do not have iRCop privileges."));
		return;
	} else {
		for (unsigned int i = 0; i < config["links"].size(); i++) {
			std::string ip = config["links"][i]["ip"].as<std::string>();
			std::string port = config["links"][i]["port"].as<std::string>();
			for (Server *server : Servers) {
				if (server->ip == ip) {
					if (Server::IsConected(server->ip) == true) {
						SendAsServer("461 " + mNickName + " :" + server->ip + ":" + port + " " + server->name + " ( \0033CONNECTED\003 )");
						break;
					} else {
						SendAsServer("461 " + mNickName + " :" + ip + " ( \0034DISCONNECTED\003 )");
						break;
					}
				}
			}
		}
	}
}
	
void LocalUser::do_cmd_connect(std::string message) {
	std::vector<std::string> results;
	Utils::split(message, results, " ");
	if (results.size() < 2) {
		SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "More data is needed."));
		return;
	} else if (getMode('o') == false) {
		SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You do not have iRCop privileges."));
		return;
	} else if (Server::IsAServer(results[1]) == false) {
		SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The server is not listed in config."));
		return;
	} else if (Server::IsConected(results[1]) == true) {
		SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The server is already connected."));
		return;
	} else {
		SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "Connecting ..."));
		for (unsigned int i = 0; i < config["links"].size(); i++) {
			if (config["links"][i]["ip"].as<std::string>() == results[1]) {
				Server *srv = new Server(config["links"][i]["ip"].as<std::string>(), config["links"][i]["port"].as<std::string>());
				Servers.insert(srv);
				srv->send("BURST");
				Server::sendBurst(srv);
			}
		}
	}
}
			

void LocalUser::do_cmd_squit(std::string message) {
	std::vector<std::string> results;
	Utils::split(message, results, " ");
	if (results.size() < 2) {
		SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "More data is needed."));
		return;
	} else if (getMode('o') == false) {
		SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You do not have iRCop privileges."));
		return;
	} else if (Server::Exists(results[1]) == false) {
		SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The server is not connected."));
		return;
	} else if (strcasecmp(results[1].c_str(), config["serverName"].as<std::string>().c_str()) == 0) {
		SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You can not make an SQUIT to your own server."));
		return;
	} else {
		Server::SQUIT(results[1], true, true);
		SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The server has been disconnected."));
		return;
	}
}
*/
