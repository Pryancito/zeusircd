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

#include <string>
#include <iterator>
#include <algorithm>
#include <mutex>

std::mutex log_mtx;
extern time_t encendido;
extern OperSet miRCOps;
extern Memos MemoMsg;
std::map<std::string, unsigned int> bForce;
extern std::set <Server*> Servers;

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
	if (config->Getvalue("serverName") == config->Getvalue("hub")) {
		time_t now = time(0);
		struct tm tm;
		localtime_r(&now, &tm);
		char date[32];
		strftime(date, sizeof(date), "%r %d-%m-%Y", &tm);
		std::ofstream fileLog("ircd.log", std::ios::out | std::ios::app);
		if (fileLog.fail()) {
			if (chan)
				chan->broadcast(":" + config->Getvalue("operserv") + " PRIVMSG #debug :Error opening log file.");
			log_mtx.unlock();
			return;
		}
		fileLog << date << " -> " << message << std::endl;
		fileLog.close();
	}
	if (chan) {
		chan->broadcast(":" + config->Getvalue("operserv") + " PRIVMSG #debug :" + message);
		Server::Send("PRIVMSG " + config->Getvalue("operserv") + " #debug " + message);
	}
	log_mtx.unlock();
}

bool LocalUser::checkstring (const std::string &str) {
	if (str.length() == 0)
		return false;
	for (unsigned int i = 0; i < str.length(); i++)
		if (!std::isalnum(str[i]))
			return false;
	return true;
}

bool LocalUser::checknick (const std::string &nick) {
	if (nick.length() == 0)
		return false;
	if (!std::isalpha(nick[0]))
		return false;
	if (nick.find("'") != std::string::npos || nick.find("\"") != std::string::npos || nick.find(";") != std::string::npos
		|| nick.find("@") != std::string::npos || nick.find("*") != std::string::npos || nick.find("/") != std::string::npos
		|| nick.find(",") != std::string::npos)
		return false;
	return true;
}

bool LocalUser::checkchan (const std::string &chan) {
	if (chan.length() == 0)
		return false;
	if (chan[0] != '#')
		return false;
	if (chan.find("'") != std::string::npos || chan.find("\"") != std::string::npos || chan.find(";") != std::string::npos
		|| chan.find("@") != std::string::npos || chan.find("*") != std::string::npos || chan.find("/") != std::string::npos
		|| chan.find(",") != std::string::npos)
		return false;
	return true;
}

void LocalUser::Parse(std::string message)
{
	if (quit == true)
		return;
	trim(message);
	std::vector<std::string> results;
	
	Config::split(message, results, " \t");
	if (results.size() == 0)
		return;
	
	std::string cmd = results[0];
	std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
	
	if (cmd == "NICK") {
		if (results.size() < 2) {
			SendAsServer("431 " + mNickName + " :" + Utils::make_string(mLang, "No nickname: [ /nick yournick ]"));
			return;
		}
		
		if (results[1][0] == ':')
			results[1] = results[1].substr(1, results[1].length());
		
		if (results[1] == mNickName)
			return;

		if (results[1].length() > (unsigned int ) stoi(config->Getvalue("nicklen"))) {
			SendAsServer("432 " + mNickName + " :" + Utils::make_string(mLang, "Nick too long."));
			return;
		}

		if (OperServ::IsSpam(results[1], "N") == true && OperServ::IsSpam(results[1], "E") == false) {
			SendAsServer("432 " + mNickName + " :" + Utils::make_string(mLang, "Your nick is marked as SPAM."));
			return;
		}

		if (mNickName != "")
			if (canchangenick() == false) {
				SendAsServer("432 " + mNickName + " :" + Utils::make_string(mLang, "You cannot change your nick."));
				return;
			}

		if (strcasecmp(results[1].c_str(), mNickName.c_str()) == 0) {
			cmdNick(results[1]);
			return;
		}

		std::string nickname = results[1];
		std::string password = "";
		
		if (nickname.find("!") != std::string::npos || nickname.find(":") != std::string::npos) {
			std::vector<std::string> nickpass;
			Config::split(nickname, nickpass, ":!");
			nickname = nickpass[0];
			password = nickpass[1];
		} else if (!PassWord.empty())
			password = PassWord;
		
		if (nickname == mNickName)
			return;
			
		if (checknick(nickname) == false) {
			SendAsServer("432 " + nickname + " :" + Utils::make_string(mLang, "The nick contains no-valid characters."));
			return;
		}
		
		if (NickServ::IsRegistered(nickname) == true && password == "") {
			Send(":" + config->Getvalue("nickserv") + " NOTICE " + nickname + " :" + Utils::make_string(mLang, "You need a password: [ /nick yournick:yourpass ]"));
			return;
		}
		
		if ((bForce.find(nickname)) != bForce.end()) {
			if (bForce.count(nickname) >= 7) {
					Send(":" + config->Getvalue("nickserv") + " NOTICE " + nickname + " :" + Utils::make_string(mLang, "Too much identify attempts for this nick. Try in 1 hour."));
					return;
			}
		}
		
		if (NickServ::IsRegistered(nickname) == true && NickServ::Login(nickname, password) == true) {
			bForce[nickname] = 0;
			LocalUser *lu = Mainframe::instance()->getLocalUserByName(nickname);
			if (lu != nullptr) {
				lu->Exit(true);
			} else {
				RemoteUser *ru = Mainframe::instance()->getRemoteUserByName(nickname);
				if (ru != nullptr) {
					ru->QUIT();
					Server::Send("QUIT " + ru->mNickName);
				}
			}
			if (getMode('r') == false) {
				setMode('r', true);
				SendAsServer("MODE " + nickname + " +r");
				Server::Send("UMODE " + nickname + " +r");
			}
			std::string lang = NickServ::GetLang(nickname);
			if (lang != "")
				mLang = lang;
			cmdNick(nickname);
			Send(":" + config->Getvalue("nickserv") + " NOTICE " + nickname + " :" + Utils::make_string(mLang, "Welcome home."));
			NickServ::UpdateLogin(this);
			return;
		} else if (NickServ::Login(nickname, password) == false && NickServ::IsRegistered(nickname) == true) {
			if (bForce.count(nickname) > 0) {
				bForce[nickname]++;
				Send(":" + config->Getvalue("nickserv") + " NOTICE " + nickname + " :" + Utils::make_string(mLang, "Wrong password."));
				return;
			} else {
				bForce[nickname] = 1;
				Send(":" + config->Getvalue("nickserv") + " NOTICE " + nickname + " :" + Utils::make_string(mLang, "Wrong password."));
				return;
			}
		} else if (Mainframe::instance()->doesNicknameExists(nickname)) {
			SendAsServer("433 " + nickname + " " + nickname + " :" + Utils::make_string(mLang, "The nick is used by somebody."));
			return;
		}
		
		if (getMode('r') == true && NickServ::IsRegistered(nickname) == false) {
			cmdNick(nickname);
			if (getMode('r') == true) {
				setMode('r', false);
				SendAsServer("MODE " + mNickName + " -r");
				Server::Send("UMODE " + mNickName + " -r");
			}
			return;
		}
		cmdNick(nickname);
	}

	else if (cmd == "USER") {
		std::string ident;
		if (results.size() < 5) {
			SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "More data is needed."));
			return;
		} if (results[1].length() > 10)
			ident = results[1].substr(0, 9);
		else
			ident = results[1];
		mIdent = ident;
		Server::Send("SUSER " + mNickName + " " + ident);
		return;
	}
	
	else if (cmd == "PASS") {
		if (results.size() < 2) {
			SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "More data is needed."));
			return;
		}
		PassWord = results[1];
		return;
	}

	else if (cmd == "QUIT") {
		quit = true;
		Close();
	}

	else if (cmd == "RELEASE") {
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

	else if (cmd == "JOIN") {
		if (!bSentNick) {
			SendAsServer("461 ZeusiRCd :" + Utils::make_string(mLang, "You havent used the NICK command yet, you have limited access."));
			return;
		}
		if (results.size() < 2) {
			SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "More data is needed."));
			return;
		}
		std::vector<std::string>  x;
		int j = 2;
		Config::split(results[1], x, ",");
		for (unsigned int i = 0; i < x.size(); i++) {
			if (checkchan(x[i]) == false) {
				SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The channel contains no-valid characters."));
				continue;
			} else if (x[i].size() < 2) {
				SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The channel name is empty."));
				continue;
			} else if (x[i].length() > (unsigned int )stoi(config->Getvalue("chanlen"))) {
				SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The channel name is too long."));
				continue;
			} else if (Channs() >= stoi(config->Getvalue("maxchannels")) && OperServ::IsException(mHost, "channel") == 0) {
				SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You enter in too much channels."));
				continue;
			} else if (OperServ::IsException(mHost, "channel") > 0 && Channs() >= OperServ::IsException(mHost, "channel")) {
				SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You enter in too much channels."));
				continue;
			} else if (ChanServ::IsRegistered(x[i]) == false) {
				Channel* chan = Mainframe::instance()->getChannelByName(x[i]);
				if (chan) {
					if (chan->hasUser(this) == true) {
						SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You are already on this channel."));
						continue;
					}
					cmdJoin(chan);
					Server::Send("SJOIN " + mNickName + " " + chan->name() + " +x");
				} else {
					Channel *chan = new Channel(this, x[i]);
					if (chan) {
						Mainframe::instance()->addChannel(chan);
						cmdJoin(chan);
						Server::Send("SJOIN " + mNickName + " " + chan->name() + " +x");
					}
				}
			} else {
				Channel* chan = Mainframe::instance()->getChannelByName(x[i]);
				if (ChanServ::HasMode(x[i], "ONLYREG") == true && getMode('r') == false && getMode('o') == false) {
					SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The entrance is only allowed to registered nicks."));
					continue;
				} else if (ChanServ::HasMode(x[i], "ONLYSECURE") == true && getMode('z') == false && getMode('o') == false) {
					SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The entrance is only allowed to SSL users."));
					continue;
				} else if (ChanServ::HasMode(x[i], "ONLYWEB") == true && getMode('w') == false && getMode('o') == false) {
					SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The entrance is only allowed to WebChat users."));
					continue;
				} else if (ChanServ::HasMode(x[i], "ONLYACCESS") == true && ChanServ::Access(mNickName, x[i]) == 0 && getMode('o') == false) {
					SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The entrance is only allowed to accessed users."));
					continue;
				} else if (ChanServ::HasMode(x[i], "COUNTRY") == true && ChanServ::CanGeoIP(this, x[i]) == false && getMode('o') == false) {
					SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The entrance is not allowed to users from your country."));
					continue;
				} else {
					if (ChanServ::IsAKICK(mNickName + "!" + mIdent + "@" + mCloak, x[i]) == true && getMode('o') == false) {
						SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You got AKICK on this channel, you cannot pass."));
						continue;
					}
					if (ChanServ::IsAKICK(mNickName + "!" + mIdent + "@" + mvHost, x[i]) == true && getMode('o') == false) {
						SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You got AKICK on this channel, you cannot pass."));
						continue;
					}
					if (ChanServ::IsKEY(x[i]) == true && getMode('o') == false) {
						if (results.size() < 3) {
							SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "I need more data: [ /join #channel password ]"));
							continue;
						} else if (ChanServ::CheckKEY(x[i], results[j]) == false) {
							SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "Wrong password."));
							continue;
						} else
							j++;
					}
				} if (chan) {
					if (chan->hasUser(this) == true) {
						SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You are already on this channel."));
						continue;
					} else if (chan->IsBan(mNickName + "!" + mIdent + "@" + mCloak) == true && getMode('o') == false) {
						SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You are banned, cannot pass."));
						continue;
					} else if (chan->IsBan(mNickName + "!" + mIdent + "@" + mvHost) == true && getMode('o') == false) {
						SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You are banned, cannot pass."));
						continue;
					} else if (chan->isonflood() == true && getMode('o') == false) {
						SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The channel is on flood, you cannot pass."));
						continue;
					} else {
						cmdJoin(chan);
						Server::Send("SJOIN " + mNickName + " " + chan->name() + " +x");
						ChanServ::DoRegister(this, chan);
						ChanServ::CheckModes(this, chan->name());
						chan->increaseflood();
					}
				} else {
					chan = new Channel(this, x[i]);
					if (chan) {
						Mainframe::instance()->addChannel(chan);
						cmdJoin(chan);
						Server::Send("SJOIN " + mNickName + " " + chan->name() + " +x");
						ChanServ::DoRegister(this, chan);
						ChanServ::CheckModes(this, chan->name());
						chan->increaseflood();
					}
				}
			}
		}
		return;
	}
	
	else if (cmd == "PART") {
		if (!bSentNick) {
			SendAsServer("461 ZeusiRCd :" + Utils::make_string(mLang, "You havent used the NICK command yet, you have limited access."));
			return;
		}
		else if (results.size() < 2) return;
		Channel* chan = Mainframe::instance()->getChannelByName(results[1]);
		if (chan) {
			if (chan->hasUser(this) == false) {
				SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You are not into the channel."));
				return;
			}
			chan->increaseflood();
			cmdPart(chan);
			Server::Send("SPART " + mNickName + " " + chan->name());
		}
		return;
	}

	else if (cmd == "TOPIC") {
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
		return;
	}

	else if (cmd == "LIST") {
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
		return;
	}

	else if (cmd == "PRIVMSG" || cmd == "NOTICE") {
		if (!bSentNick) {
			SendAsServer("461 ZeusiRCd :" + Utils::make_string(mLang, "You havent used the NICK command yet, you have limited access."));
			return;
		}
		else if (results.size() < 3) return;
		std::string mensaje = "";
		for (unsigned int i = 2; i < results.size(); ++i) { mensaje.append(results[i] + " "); }
		trim(mensaje);
		if (results[1][0] == '#') {
			Channel* chan = Mainframe::instance()->getChannelByName(results[1]);
			if (chan) {
				if (ChanServ::HasMode(chan->name(), "MODERATED") && !chan->isOperator(this) && !chan->isHalfOperator(this) && !chan->isVoice(this) && getMode('o') == false) {
					SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The channel is moderated, you cannot speak."));
					return;
				} else if (chan->isonflood() == true && ChanServ::Access(mNickName, chan->name()) == 0) {
					SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The channel is on flood, you cannot speak."));
					return;
				} else if (chan->hasUser(this) == false) {
					SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You are not into the channel."));
					return;
				} else if (chan->IsBan(mNickName + "!" + mIdent + "@" + mCloak) == true) {
					SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You are banned, cannot speak."));
					return;
				} else if (chan->IsBan(mNickName + "!" + mIdent + "@" + mvHost) == true) {
					SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You are banned, cannot speak."));
					return;
				} else if (OperServ::IsSpam(mensaje, "C") == true && OperServ::IsSpam(mensaje, "E") == false && getMode('o') == false && strcasecmp(chan->name().c_str(), "#spam") != 0) {
					Oper oper;
					oper.GlobOPs(Utils::make_string("", "Nickname %s try to make SPAM into channel: %s", mNickName.c_str(), chan->name().c_str()));
					SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The message of channel %s contains SPAM.", chan->name().c_str()));
					return;
				}
				chan->increaseflood();
				chan->broadcast_except_me(mNickName,
					messageHeader()
					+ cmd + " "
					+ chan->name() + " "
					+ mensaje);
				Server::Send(cmd + " " + mNickName + "!" + mIdent + "@" + mvHost + " " + chan->name() + " " + mensaje);
			}
		}
		else {
			User* target = Mainframe::instance()->getUserByName(results[1]);
			if (OperServ::IsSpam(mensaje, "P") == true && OperServ::IsSpam(mensaje, "E") == false && getMode('o') == false && target) {
				Oper oper;
				oper.GlobOPs(Utils::make_string("", "Nickname %s try to make SPAM to nick: %s", mNickName.c_str(), target->mNickName.c_str()));
				SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "Message to nick %s contains SPAM.", target->mNickName.c_str()));
				return;
			} else if (NickServ::GetOption("NOCOLOR", results[1]) == true && target && mensaje.find("\003") != std::string::npos) {
				SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "Message to nick %s contains colours.", target->mNickName.c_str()));
				return;
			} else if (target && NickServ::GetOption("ONLYREG", results[1]) == true && getMode('r') == false) {
				SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The nick %s only can receive messages from registered nicks.", target->mNickName.c_str()));
				return;
			}
			LocalUser* tuser = Mainframe::instance()->getLocalUserByName(results[1]);
			if (tuser) {
				if (tuser->bAway == true) {
					Send(tuser->messageHeader()
						+ "NOTICE "
						+ mNickName + " :AWAY " + tuser->mAway);
					tuser->Send(messageHeader()
						+ cmd + " "
						+ tuser->mNickName + " "
						+ mensaje);
				}
				return;
			} else {
				RemoteUser *u = Mainframe::instance()->getRemoteUserByName(results[1]);
				if (u->bAway == true) {
					Send(u->messageHeader()
						+ "NOTICE "
						+ mNickName + " :AWAY " + u->mAway);
				}
				Server::Send(cmd + " " + mNickName + "!" + mIdent + "@" + mvHost + " " + u->mNickName + " " + mensaje);
				return;
			} if (!target && NickServ::IsRegistered(results[1]) == true && NickServ::MemoNumber(results[1]) < 50 && NickServ::GetOption("NOMEMO", results[1]) == 0) {
				Memo *memo = new Memo();
					memo->sender = mNickName;
					memo->receptor = results[1];
					memo->time = time(0);
					memo->mensaje = mensaje;
				MemoMsg.insert(memo);
				Send(":NiCK!*@* NOTICE " + mNickName + " :" + Utils::make_string(mLang, "The nick is offline, MeMo has been sent."));
				Server::Send("MEMO " + memo->sender + " " + memo->receptor + " " + std::to_string(memo->time) + " " + memo->mensaje);
				return;
			} else
				SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The nick doesnt exists or cannot receive messages."));
		}
		return;
	}

	else if (cmd == "KICK") {
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
		return;
	}
	
	else if (cmd == "NAMES") {
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
	
	else if (cmd == "WHO") {
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

	else if (cmd == "AWAY") {
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

	else if (cmd == "OPER") {
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

	else if (cmd == "PING") {
		bPing = time(0);
		SendAsServer("PONG " + config->Getvalue("serverName") + " :" + (results.size() > 1 ? results[1] : ""));
	}
	
	else if (cmd == "PONG") { bPing = time(0); }

	else if (cmd == "USERHOST") { return; }

	else if (cmd == "CAP") {
		if (results.size() < 2) return;
		else if (results[1] == "LS" || results[1] == "LIST")
			sendCAP(results[1]);
		else if (results[1] == "REQ")
			Request(message);
		else if (results[1] == "END")
			recvEND();
	}

	else if (cmd == "WEBIRC") {
		if (results.size() < 5) {
			SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "More data is needed."));
			return;
		} else if (results[1] == config->Getvalue("cgiirc")) {
			mHost = results[4];
			return;
		} else {
			Close();
		}
	}

	else if (cmd == "STATS" || cmd == "LUSERS") {
		SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "There are \002%s\002 users and \002%s\002 channels.", std::to_string(Mainframe::instance()->countusers()).c_str(), std::to_string(Mainframe::instance()->countchannels()).c_str()));
		SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "There are \002%s\002 registered nicks and \002%s\002 registered channels.", std::to_string(NickServ::GetNicks()).c_str(), std::to_string(ChanServ::GetChans()).c_str()));
		SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "There are \002%s\002 connected iRCops.", std::to_string(Oper::Count()).c_str()));
		SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "There are \002%s\002 connected servers.", std::to_string(Server::count()).c_str()));
		return;
	}
	
	else if (cmd == "OPERS") {
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
		return;
	}
	
	else if (cmd == "UPTIME") {
		SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "This server started as long as: %s", Utils::Time(encendido).c_str()));
		return;
	}

	else if (cmd == "VERSION") {
		SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "Version of ZeusiRCd: %s", config->version.c_str()));
		if (getMode('o') == true)
			SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "Version of DataBase: %s", DB::GetLastRecord().c_str()));
		return;
	}

	else if (cmd == "REHASH") {
		if (getMode('o') == false) {
			SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "You do not have iRCop privileges."));
			return;
		} else {
			config->conf.clear();
			config->Cargar();
			SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "The config has been reloaded."));
			return;
		}
	}
	
	else if (cmd == "MODE") {
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
								Send(":" + config->Getvalue("chanserv") + " NOTICE " + mNickName + " :" + Utils::make_string(mLang, "The BAN already exist."));
							} else {
								chan->setBan(results[3+j], mNickName);
								chan->broadcast(messageHeader() + "MODE " + chan->name() + " +b " + results[3+j]);
								Send(":" + config->Getvalue("chanserv") + " NOTICE " + mNickName + " :" + Utils::make_string(mLang, "The BAN has been set."));
								Server::Send("CMODE " + mNickName + " " + chan->name() + " +b " + results[3+j]);
							}
						} else {
							if (chan->IsBan(maskara) == false) {
								Send(":" + config->Getvalue("chanserv") + " NOTICE " + mNickName + " :" + Utils::make_string(mLang, "The BAN does not exist."));
							} else {
								BanSet bans = chan->bans();
								BanSet::iterator it = bans.begin();
								for (; it != bans.end(); ++it)
									if ((*it)->mask() == results[3+j]) {
										chan->UnBan((*it));
										chan->broadcast(messageHeader() + "MODE " + chan->name() + " -b " + results[3+j]);
										Send(":" + config->Getvalue("chanserv") + " NOTICE " + mNickName + " :" + Utils::make_string(mLang, "The BAN has been deleted."));
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
								Send(":" + config->Getvalue("chanserv") + " NOTICE " + mNickName + " :" + Utils::make_string(mLang, "The BAN already exist."));
							} else {
								chan->setpBan(results[3+j], mNickName);
								chan->broadcast(messageHeader() + "MODE " + chan->name() + " +B " + results[3+j]);
								Send(":" + config->Getvalue("chanserv") + " NOTICE " + mNickName + " :" + Utils::make_string(mLang, "The BAN has been set."));
								Server::Send("CMODE " + mNickName + " " + chan->name() + " +B " + results[3+j]);
							}
						} else {
							if (chan->IsBan(maskara) == false) {
								Send(":" + config->Getvalue("chanserv") + " NOTICE " + mNickName + " :" + Utils::make_string(mLang, "The BAN does not exist."));
							} else {
								pBanSet bans = chan->pbans();
								pBanSet::iterator it = bans.begin();
								for (; it != bans.end(); ++it)
									if ((*it)->mask() == results[3+j]) {
										chan->UnpBan((*it));
										chan->broadcast(messageHeader() + "MODE " + chan->name() + " -B " + results[3+j]);
										Send(":" + config->Getvalue("chanserv") + " NOTICE " + mNickName + " :" + Utils::make_string(mLang, "The BAN has been deleted."));
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
		return;
	}
	
	else if (cmd == "WHOIS") {
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
		return;
	}

	else if (cmd == "SERVERS") {
		if (getMode('o') == false) {
			SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You do not have iRCop privileges."));
			return;
		} else {
			for (unsigned int i = 0; config->Getvalue("link["+std::to_string(i)+"]ip").length() > 0; i++) {
				std::string ip = config->Getvalue("link["+std::to_string(i)+"]ip");
				std::string port = config->Getvalue("link["+std::to_string(i)+"]port");
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
			return;
		}
	}
	
	else if (cmd == "CONNECT") {
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
			for (unsigned int i = 0; config->Getvalue("link["+std::to_string(i)+"]ip").length() > 0; i++) {
				if (config->Getvalue("link["+std::to_string(i)+"]ip") == results[1]) {
					Server *srv = new Server(config->Getvalue("link["+std::to_string(i)+"]ip"), config->Getvalue("link["+std::to_string(i)+"]port"));
					Servers.insert(srv);
					srv->send("BURST");
					Server::sendBurst(srv);
				}
			}
		}
	}
			

	else if (cmd == "SQUIT") {
		if (results.size() < 2) {
			SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "More data is needed."));
			return;
		} else if (getMode('o') == false) {
			SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You do not have iRCop privileges."));
			return;
		} else if (Server::Exists(results[1]) == false) {
			SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The server is not connected."));
			return;
		} else if (strcasecmp(results[1].c_str(), config->Getvalue("serverName").c_str()) == 0) {
			SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You can not make an SQUIT to your own server."));
			return;
		} else {
			Server::SQUIT(results[1], true, true);
			SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "The server has been disconnected."));
			return;
		}
	}
	
	else if (cmd == "NICKSERV" || cmd == "NS") {
		if (results.size() < 2) {
			Send(":" + config->Getvalue("nickserv") + " NOTICE " + mNickName + " :" + Utils::make_string(mLang, "More data is needed."));
			return;
		} else if (!bSentNick) {
			SendAsServer("461 ZeusiRCd :" + Utils::make_string(mLang, "You havent used the NICK command yet, you have limited access."));
			return;
		} else if (cmd == "NICKSERV"){
			NickServ::Message(this, message.substr(9));
			return;
		} else if (cmd == "NS"){
			NickServ::Message(this, message.substr(3));
			return;
		}
	} 

	else if (cmd == "CHANSERV" || cmd == "CS") {
		if (results.size() < 2) {
			Send(":" + config->Getvalue("chanserv") + " NOTICE " + mNickName + " :" + Utils::make_string(mLang, "More data is needed."));
			return;
		} else if (!bSentNick) {
			SendAsServer("461 ZeusiRCd :" + Utils::make_string(mLang, "You havent used the NICK command yet, you have limited access."));
			return;
		} else if (cmd == "CHANSERV"){
			ChanServ::Message(this, message.substr(9));
			return;
		} else if (cmd == "CS"){
			ChanServ::Message(this, message.substr(3));
			return;
		}
	} 

	else if (cmd == "HOSTSERV" || cmd == "HS") {
		if (results.size() < 2) {
			Send(":" + config->Getvalue("hostserv") + " NOTICE " + mNickName + " :" + Utils::make_string(mLang, "More data is needed."));
			return;
		} else if (!bSentNick) {
			SendAsServer("461 ZeusiRCd :" + Utils::make_string(mLang, "You havent used the NICK command yet, you have limited access."));
			return;
		} else if (cmd == "HOSTSERV"){
			HostServ::Message(this, message.substr(9));
			return;
		} else if (cmd == "HS"){
			HostServ::Message(this, message.substr(3));
			return;
		}
	}

	else if (cmd == "OPERSERV" || cmd == "OS") {
		if (getMode('o') == false) {
			Send(":" + config->Getvalue("serverName") + " 461 " + mNickName + " :" + Utils::make_string(mLang, "You do not have iRCop privileges."));
			return;
		} else if (results.size() < 2) {
			Send(":" + config->Getvalue("operserv") + " NOTICE " + mNickName + " :" + Utils::make_string(mLang, "More data is needed."));
			return;
		} else if (!bSentNick) {
			SendAsServer("461 ZeusiRCd :" + Utils::make_string(mLang, "You havent used the NICK command yet, you have limited access."));
			return;
		} else if (cmd == "OPERSERV"){
			OperServ::Message(this, message.substr(9));
			return;
		} else if (cmd == "OS"){
			OperServ::Message(this, message.substr(3));
			return;
		}
	}

	else {
		SendAsServer("421 " + mNickName + " :" + Utils::make_string(mLang, "Unknown command."));
		return;
	}
}
