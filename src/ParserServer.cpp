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
#include "services.h"
#include "sha256.h"

extern OperSet miRCOps;
extern Memos MemoMsg;
extern std::set <Server*> Servers;

std::string& ltrim(std::string& str, const std::string& chars = "\t\n\v\f\r ");
std::string& rtrim(std::string& str, const std::string& chars = "\t\n\v\f\r ");
std::string& trim(std::string& str, const std::string& chars = "\t\n\v\f\r ");

void Server::Parse(std::string message)
{
	if (message.length() == 0) return;
	std::vector<std::string>  x;
	Config::split(message, x, " \t");
	std::string cmd = x[0];
	std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
	Oper oper;
	Ping();
	if (cmd == "HUB") {
		if (x.size() < 2) {
			oper.GlobOPs(Utils::make_string("", "HUB is not present, closing connection."));
			Close();
			return;
		} else if (x[1] != config->Getvalue("hub")) {
			oper.GlobOPs(Utils::make_string("", "Closing connection. HUB missmatch. ( %s > %s )", config->Getvalue("hub").c_str(), x[1].c_str()));
			Close();
			return;
		}
	} else if (cmd == "VERSION") {
		if (x.size() < 2) {
			oper.GlobOPs(Utils::make_string("", "Error in DataBases, closing connection."));
			Close();
			return;
		} else if (DB::GetLastRecord() != x[1] && Server::HUBExiste() == true) {
			oper.GlobOPs(Utils::make_string("", "Sincronyzing DataBases."));
			int syn = DB::Sync(this, x[1]);
			oper.GlobOPs(Utils::make_string("", "DataBases syncronized, %s records updated.", std::to_string(syn).c_str()));
			return;
		}
	} else if (cmd == "SERVER") {
		if (x.size() < 2) {
			oper.GlobOPs(Utils::make_string("", "serverName Error. Closing connection."));
			Close();
			return;
		} else {
			name = x[1];
			ip = remoteip();
			Servers.insert(this);
			return;
		}
	} else if (cmd == "DB") {
		std::string sql = message.substr(36);
		DB::SQLiteNoReturn(sql);
		DB::AlmacenaDB(message);
	} else if (cmd == "SNICK") {
		if (x.size() < 8) {
			oper.GlobOPs(Utils::make_string("", "ERROR: invalid %s.", "SNICK"));
			return;
		}
		LocalUser* target = Mainframe::instance()->getLocalUserByName(x[1]);
		if (target) {
			target->Close();
		} else {
			RemoteUser *user = new RemoteUser(x[7]);
			user->mNickName = x[1];
			user->mIdent = x[2];
			user->mHost = x[3];
			user->mCloak = x[4];
			user->mvHost = x[5];
			user->bLogin = stoi(x[6]);
			for (unsigned int i = 1; i < x[8].size(); i++) {
				if (x[8][i] == 'x') {
					break;
				} else if (x[8][i] == 'o') {
					user->setMode('o', true);
					miRCOps.insert(x[1]);
				} else if (x[8][i] == 'w') {
					user->setMode('w', true);
				} else if (x[8][i] == 'z') {
					user->setMode('z', true);
				} else if (x[8][i] == 'r') {
					user->setMode('r', true);
				}
			}
			if (!Mainframe::instance()->addRemoteUser(user, x[1]))
				oper.GlobOPs(Utils::make_string("", "ERROR: Can not introduce the user %s with SNICK command.", x[1].c_str()));
		}
	}  else if (cmd == "SUSER") {
		if (x.size() < 3) {
			oper.GlobOPs(Utils::make_string("", "ERROR: invalid %s.", "SUSER"));
			return;
		}
		User* target = Mainframe::instance()->getUserByName(x[1]);
		if (target) {
			target->mIdent = x[2];
		}
	} else if (cmd == "SBAN") {
		if (x.size() < 5) {
			oper.GlobOPs(Utils::make_string("", "ERROR: invalid %s.", "SBAN"));
			return;
		}
		Channel* chan = Mainframe::instance()->getChannelByName(x[1]);
		if (chan) {
			if (chan->IsBan(x[2]) == false)
				chan->SBAN(x[2], x[3], x[4]);
		} else
			oper.GlobOPs(Utils::make_string("", "ERROR: invalid %s.", "SBAN"));
	} else if (cmd == "SPBAN") {
		if (x.size() < 5) {
			oper.GlobOPs(Utils::make_string("", "ERROR: invalid %s.", "SPBAN"));
			return;
		}
		Channel* chan = Mainframe::instance()->getChannelByName(x[1]);
		if (chan) {
			if (chan->IsBan(x[2]) == false)
				chan->SPBAN(x[2], x[3], x[4]);
		} else
			oper.GlobOPs(Utils::make_string("", "ERROR: invalid %s.", "SPBAN"));
	} else if (cmd == "SJOIN") {
		if (x.size() < 4) {
			oper.GlobOPs(Utils::make_string("", "ERROR: invalid %s.", "SJOIN"));
			return;
		}
		Channel* chan = Mainframe::instance()->getChannelByName(x[2]);
		RemoteUser* user = Mainframe::instance()->getRemoteUserByName(x[1]);
		if (user) {
			if (chan) {
				user->SJOIN(chan);
			} else {
				chan = new Channel(user, x[2]);
				if (chan) {
					Mainframe::instance()->addChannel(chan);
					user->SJOIN(chan);
				}
			}
			if (x[3][1] == 'o') {
				chan->giveOperator(user);
				chan->broadcast(":" + config->Getvalue("serverName") + " MODE " + chan->name() + " +o " + user->mNickName);
			} else if (x[3][1] == 'h') {
				chan->giveHalfOperator(user);
				chan->broadcast(":" + config->Getvalue("serverName") + " MODE " + chan->name() + " +h " + user->mNickName);
			} else if (x[3][1] == 'v') {
				chan->giveVoice(user);
				chan->broadcast(":" + config->Getvalue("serverName") + " MODE " + chan->name() + " +v " + user->mNickName);
			}
			return;
		}
	} else if (cmd == "SPART") {
		if (x.size() < 3) {
			oper.GlobOPs(Utils::make_string("", "ERROR: invalid %s.", "SPART"));
			return;
		}
		Channel* chan = Mainframe::instance()->getChannelByName(x[2]);
		RemoteUser* user = Mainframe::instance()->getRemoteUserByName(x[1]);
		if (user)
			user->SPART(chan);
	} else if (cmd == "UMODE") {
		if (x.size() < 3) {
			oper.GlobOPs(Utils::make_string("", "ERROR: invalid %s.", "UMODE"));
			return;
		}
		RemoteUser* user = Mainframe::instance()->getRemoteUserByName(x[1]);
		if (!user) {
			return;
		}
		bool add = false;
		if (x[2][0] == '+')
			add = true;
		if (x[2][1] == 'o') {
			user->setMode('o', add);
			if (add) miRCOps.insert(user->mNickName);
			else miRCOps.erase(user->mNickName);
		} else if (x[2][1] == 'w')
			user->setMode('w', add);
		else if (x[2][1] == 'r')
			user->setMode('r', add);
		else if (x[2][1] == 'z')
			user->setMode('z', add);
	} else if (cmd == "CMODE") {
		if (x.size() < 4) {
			oper.GlobOPs(Utils::make_string("", "ERROR: invalid %s.", "CMODE"));
			return;
		}
		{
			LocalUser* target = nullptr;
			if (x.size() == 5)
				target = Mainframe::instance()->getLocalUserByName(x[4]);
			Channel* chan = Mainframe::instance()->getChannelByName(x[2]);
			bool add = false;
			if (x[3][0] == '+')
				add = true;
			if ((!target && x[3][1] != 'b' && x[3][1] != 'r') || !chan) {
				goto remote;
			} if (x[3][1] == 'o' && add == true) {
				chan->giveOperator(target);
				chan->broadcast(":" + x[1] + " MODE " + chan->name() + " +o " + target->mNickName);
			} else if (x[3][1] == 'o' && add == false) {
				chan->delOperator(target);
				chan->broadcast(":" + x[1] + " MODE " + chan->name() + " -o " + target->mNickName);
			} else if (x[3][1] == 'h' && add == true) {
				chan->giveHalfOperator(target);
				chan->broadcast(":" + x[1] + " MODE " + chan->name() + " +h " + target->mNickName);
			} else if (x[3][1] == 'h' && add == false) {
				chan->delHalfOperator(target);
				chan->broadcast(":" + x[1] + " MODE " + chan->name() + " -h " + target->mNickName);
			} else if (x[3][1] == 'v' && add == true) {
				chan->giveVoice(target);
				chan->broadcast(":" + x[1] + " MODE " + chan->name() + " +v " + target->mNickName);
			} else if (x[3][1] == 'v' && add == false) {
				chan->delVoice(target);
				chan->broadcast(":" + x[1] + " MODE " + chan->name() + " -v " + target->mNickName);
			} else if (x[3][1] == 'b' && add == true) {
				chan->setBan(x[4], x[1]);
				chan->broadcast(":" + x[1] + " MODE " + chan->name() + " +b " + x[4]);
			} else if (x[3][1] == 'b' && add == false) {
				for (auto ban : chan->bans()) {
					if (ban->mask() == x[4]) {
						chan->broadcast(":" + x[1] + " MODE " + chan->name() + " -b " + x[4]);
						chan->UnBan(ban);
					}
				}
			} else if (x[3][1] == 'B' && add == true) {
				chan->setpBan(x[4], x[1]);
				chan->broadcast(":" + x[1] + " MODE " + chan->name() + " +B " + x[4]);
			} else if (x[3][1] == 'b' && add == false) {
				for (auto ban : chan->pbans()) {
					if (ban->mask() == x[4]) {
						chan->broadcast(":" + x[1] + " MODE " + chan->name() + " -B " + x[4]);
						chan->UnpBan(ban);
					}
				}
			} else if (x[3][1] == 'r' && add == true) {
				chan->setMode('r', true);
				chan->broadcast(":" + x[1] + " MODE " + chan->name() + " +r");
			} else if (x[3][1] == 'r' && add == false) {
				chan->setMode('r', false);
				chan->broadcast(":" + x[1] + " MODE " + chan->name() + " -r");
			}
			return;
		}
		{
			remote:
			RemoteUser* target = nullptr;
			if (x.size() == 5)
				target = Mainframe::instance()->getRemoteUserByName(x[4]);
			Channel* chan = Mainframe::instance()->getChannelByName(x[2]);
			bool add = false;
			if (x[3][0] == '+')
				add = true;
			if ((!target && x[3][1] != 'b' && x[3][1] != 'r') || !chan) {
				return;
			} if (x[3][1] == 'o' && add == true) {
				chan->giveOperator(target);
				chan->broadcast(":" + x[1] + " MODE " + chan->name() + " +o " + target->mNickName);
			} else if (x[3][1] == 'o' && add == false) {
				chan->delOperator(target);
				chan->broadcast(":" + x[1] + " MODE " + chan->name() + " -o " + target->mNickName);
			} else if (x[3][1] == 'h' && add == true) {
				chan->giveHalfOperator(target);
				chan->broadcast(":" + x[1] + " MODE " + chan->name() + " +h " + target->mNickName);
			} else if (x[3][1] == 'h' && add == false) {
				chan->delHalfOperator(target);
				chan->broadcast(":" + x[1] + " MODE " + chan->name() + " -h " + target->mNickName);
			} else if (x[3][1] == 'v' && add == true) {
				chan->giveVoice(target);
				chan->broadcast(":" + x[1] + " MODE " + chan->name() + " +v " + target->mNickName);
			} else if (x[3][1] == 'v' && add == false) {
				chan->delVoice(target);
				chan->broadcast(":" + x[1] + " MODE " + chan->name() + " -v " + target->mNickName);
			} else if (x[3][1] == 'b' && add == true) {
				chan->setBan(x[4], x[1]);
				chan->broadcast(":" + x[1] + " MODE " + chan->name() + " +b " + x[4]);
			} else if (x[3][1] == 'b' && add == false) {
				for (auto ban : chan->bans()) {
					if (ban->mask() == x[4]) {
						chan->broadcast(":" + x[1] + " MODE " + chan->name() + " -b " + x[4]);
						chan->UnBan(ban);
					}
				}
			} else if (x[3][1] == 'r' && add == true) {
				chan->setMode('r', true);
				chan->broadcast(":" + x[1] + " MODE " + chan->name() + " +r");
			} else if (x[3][1] == 'r' && add == false) {
				chan->setMode('r', false);
				chan->broadcast(":" + x[1] + " MODE " + chan->name() + " -r");
			}
			return;
		}
	} else if (cmd == "QUIT") {
		if (x.size() < 2) {
			oper.GlobOPs(Utils::make_string("", "ERROR: invalid %s.", "QUIT"));
			return;
		}
		RemoteUser* target = Mainframe::instance()->getRemoteUserByName(x[1]);
		if (target)
			target->QUIT();
	} else if (cmd == "NICK") {
		if (x.size() < 3) {
			oper.GlobOPs(Utils::make_string("", "ERROR: invalid %s.", "NICK"));
			return;
		}
		if(Mainframe::instance()->changeRemoteNickname(x[1], x[2])) {
			RemoteUser* user = Mainframe::instance()->getRemoteUserByName(x[2]);
			if (!user) {
				oper.GlobOPs(Utils::make_string("", "ERROR: invalid %s.", "NICK"));
				return;
			}
			user->NICK(x[2]);
        } else {
			oper.GlobOPs(Utils::make_string("", "ERROR: invalid %s.", "NICK"));
			return;
		}
	} else if (cmd == "PRIVMSG" || cmd == "NOTICE") {
		if (x.size() < 4) {
			oper.GlobOPs(Utils::make_string("", "ERROR: invalid %s.", "PRIVMSG|NOTICE"));
			return;
		}
		std::string mensaje = "";
		for (unsigned int i = 3; i < x.size(); ++i) { mensaje.append(x[i] + " "); }
		trim(mensaje);
		if (x[2][0] == '#') {
			Channel* chan = Mainframe::instance()->getChannelByName(x[2]);
			if (chan) {
				chan->broadcast(
					":" + x[1] + " "
					+ x[0] + " "
					+ chan->name() + " "
					+ mensaje);
			}
		} else {
			LocalUser* target = Mainframe::instance()->getLocalUserByName(x[2]);
			if (target) {
				target->Send(":" + x[1] + " "
					+ x[0] + " "
					+ target->mNickName + " "
					+ mensaje);
				return;
			}
		}
	} else if (cmd == "SKICK") {
		if (x.size() < 5) {
			oper.GlobOPs(Utils::make_string("", "ERROR: invalid %s.", "SKICK"));
			return;
		}
		std::string reason = "";
		for (unsigned int i = 4; i < x.size(); ++i) { reason.append(x[i] + " "); }
		trim(reason);
		RemoteUser*  user = Mainframe::instance()->getRemoteUserByName(x[1]);
		Channel* chan = Mainframe::instance()->getChannelByName(x[2]);
		LocalUser*  victim = Mainframe::instance()->getLocalUserByName(x[3]);
		if (chan && user && victim) {
			victim->cmdKick(user->mNickName, victim->mNickName, reason, chan);
			return;
		}
		RemoteUser*  rvictim = Mainframe::instance()->getRemoteUserByName(x[3]);
		if (chan && user && rvictim) {
			rvictim->SKICK(user->mNickName, rvictim->mNickName, reason, chan);
		}
	} else if (cmd == "AWAY") {
		if (x.size() < 2) {
			oper.GlobOPs(Utils::make_string("", "ERROR: invalid %s.", "AWAY"));
			return;
		} else if (x.size() == 2) {
			LocalUser*  user = Mainframe::instance()->getLocalUserByName(x[1]);
			if (user) {
				user->cmdAway("", false);
				return;
			}
		} else {
			LocalUser*  user = Mainframe::instance()->getLocalUserByName(x[1]);
			if (user) {
				std::string away = "";
				for (unsigned int i = 2; i < x.size(); ++i) { away.append(x[i] + " "); }
				trim(away);
				user->cmdAway(away, true);
			}
		}
	} else if (cmd == "PING") {
		Send("PONG " + config->Getvalue("serverName"));
		bPing = time(0);
	} else if (cmd == "PONG") {
		bPing = time(0);
	} else if (cmd == "MEMO") {
		if (x.size() < 5) {
			oper.GlobOPs(Utils::make_string("", "ERROR: invalid %s.", "MEMO"));
			return;
		}
		std::string mensaje = "";
		for (unsigned int i = 4; i < x.size(); ++i) { mensaje += " " + x[i]; }
		Memo *memo = new Memo();
			memo->sender = x[1];
			memo->receptor = x[2];
			memo->time = (time_t ) stoi(x[3]);
			memo->mensaje = mensaje;
		MemoMsg.insert(memo);
	} else if (cmd == "MEMODEL") {
		if (x.size() < 2) {
			oper.GlobOPs(Utils::make_string("", "ERROR: invalid %s.", "MEMODEL"));
			return;
		}
		auto it = MemoMsg.begin();
		while(it != MemoMsg.end())
			if (strcasecmp((*it)->receptor.c_str(), x[1].c_str()) == 0)
				it = MemoMsg.erase(it);
	} else if (cmd == "WEBIRC") {
		if (x.size() < 3) {
			oper.GlobOPs(Utils::make_string("", "ERROR: invalid %s.", "WEBIRC"));
			return;
		}
		LocalUser*  target = Mainframe::instance()->getLocalUserByName(x[1]);
		if (target) {
			target->mCloak = sha256(x[2]).substr(0, 16);
			target->mHost = x[2];
		}
	} else if (cmd == "VHOST") {
		if (x.size() < 3) {
			oper.GlobOPs(Utils::make_string("", "ERROR: invalid %s.", "VHOST"));
			return;
		}
		LocalUser*  target = Mainframe::instance()->getLocalUserByName(x[1]);
		if (target) {
			target->Cycle();
		}
	} else if (cmd == "SQUIT") {
		if (x.size() < 2) {
			oper.GlobOPs(Utils::make_string("", "ERROR: invalid %s.", "SQUIT"));
			return;
		} else {
			SQUIT(x[1]);
		}
	} else if (cmd == "SKILL") {
		if (x.size() < 2) {
			oper.GlobOPs(Utils::make_string("", "ERROR: invalid %s.", "SKILL"));
			return;
		} else {
			LocalUser *lu = Mainframe::instance()->getLocalUserByName(x[1]);
			if (lu != nullptr) {
				lu->Close();
			} else {
				RemoteUser *ru = Mainframe::instance()->getRemoteUserByName(x[1]);
				if (ru != nullptr) {
					ru->QUIT();
				}
			}
		}
	}
}
