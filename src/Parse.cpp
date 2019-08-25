#include "ZeusBaseClass.h"
#include "Config.h"
#include "Timer.h"
#include "Utils.h"
#include "services.h"
#include "oper.h"
#include "sha256.h"
#include "Server.h"
#include "Channel.h"
#include "mainframe.h"

#include <string>
#include <iterator>
#include <algorithm>
#include <mutex>

std::mutex log_mtx;
std::mutex quit_mtx;
TimerWheel timers;
extern time_t encendido;
extern OperSet miRCOps;
std::map<std::string, unsigned int> bForce;

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
	if (config->Getvalue("serverName") == config->Getvalue("hub")) {
		log_mtx.lock();
		Channel *chan = Channel::FindChannel("#debug");
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
		if (chan)
			chan->broadcast(":" + config->Getvalue("operserv") + " PRIVMSG #debug :" + message);
		log_mtx.unlock();
	}
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
	trim(message);
	std::vector<std::string> results;
	
	Config::split(message, results, " \t");
	
	if (results.size() == 0)
		return;
	
	bPing = time(0);
	
	std::string cmd = results[0];
	std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
	
	if (cmd == "QUIT") {
		quit = true;
		return;
	} else if (cmd == "PING") {
		bPing = time(0);
		SendAsServer("PONG " + config->Getvalue("serverName") + " :" + (results.size() > 1 ? results[1] : ""));
	} else if (cmd == "PONG") {
		bPing = time(0);
	} else if (cmd == "STATS") {
		SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "There are \002%s\002 users and \002%s\002 channels.", std::to_string(Mainframe::instance()->countusers()).c_str(), std::to_string(Mainframe::instance()->countchannels()).c_str()));
		SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "There are \002%s\002 registered nicks and \002%s\002 registered channels.", std::to_string(NickServ::GetNicks()).c_str(), std::to_string(ChanServ::GetChans()).c_str()));
		SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "There are \002%s\002 connected iRCops.", std::to_string(Oper::Count()).c_str()));
	} else if (cmd == "NICK") {
		if (results.size() < 2) {
			SendAsServer("431 " + mNickName + " :" + Utils::make_string(mLang, "No nickname: [ /nick yournick ]"));
			return;
		}
		
		if (results[1][0] == ':')
			results[1] = results[1].substr(1, results[1].length());
			
		if (results[1].length() > (unsigned int ) stoi(config->Getvalue("nicklen"))) {
			SendAsServer("432 " + mNickName + " :" + Utils::make_string(mLang, "Nick too long."));
				return;
		}
		
		std::string nickname = results[1];
		std::string password = "";
		
		if (nickname.find("!") != std::string::npos || nickname.find(":") != std::string::npos) {
			std::vector<std::string> nickpass;
			Config::split(nickname, nickpass, ":!");
			nickname = nickpass[0];
			password = nickpass[1];
		} else if (bSentPass)
			password = PassWord;

		if (nickname == mNickName)
			return;
			
		if ((bForce.find(nickname)) != bForce.end()) {
			if (bForce.count(nickname) >= 7) {
					Send(":" + config->Getvalue("nickserv") + " NOTICE " + nickname + " :" + Utils::make_string(mLang, "Too much identify attempts for this nick. Try in 1 hour."));
					return;
			}
		}

		if (!bSentNick)
		{
			if(NickServ::Login(nickname, password) == true)
			{
				bForce.erase(nickname);
				if (Mainframe::instance()->doesNicknameExists(nickname) == true)
				{
					LocalUser *user = Mainframe::instance()->getLocalUserByName(nickname);
					if (user)
					{
						user->quit = true;
						user->Close();
					} else {
						RemoteUser *user = Mainframe::instance()->getRemoteUserByName(nickname);
						if (user)
						{
							
						}
					}
				}
				if (Mainframe::instance()->addLocalUser(this, nickname)) {
					if (getMode('r') == false) {
						setMode('r', true);
						SendAsServer("MODE " + nickname + " +r");
						Server::sendall("UMODE " + nickname + " +r");
					}
					std::string lang = NickServ::GetLang(nickname);
					mLang = lang;
					mNickName = nickname;
					//if (getMode('w') == false) {
						mCloak = sha256(mHost).substr(0, 16);
					//}
					User::log(Utils::make_string("", "Nickname %s enter to irc with ip: %s", nickname.c_str(), mHost.c_str()));
					Send(":" + nickname + " NICK :"+ nickname);
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
						//SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "There are \002%s\002 connected servers.", std::to_string(Server::count()).c_str()));
						SendAsServer("422 " + mNickName + " :No MOTD");
						if (ssl == true) {
							setMode('z', true);
							SendAsServer("MODE " + mNickName + " +z");
						} if (websocket == true) {
							setMode('w', true);
							SendAsServer("MODE " + mNickName + " +w");
						} if (OperServ::IsOper(nickname) == true) {
							miRCOps.insert(nickname);
							setMode('o', true);
							SendAsServer("MODE " + mNickName + " +o");
						}
						std::string cloak = NickServ::GetvHost(mNickName);
						if (cloak != "")
							mvHost = cloak;
						else
							mvHost = mCloak;
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
						Server::sendall("SNICK " + mNickName + " " + mIdent + " " + mHost + " " + std::to_string(bLogin) + " " + mServer + " " + modos);
						NickServ::checkmemos(this);
						Send(":" + config->Getvalue("nickserv") + " NOTICE " + mNickName + " :" + Utils::make_string(mLang, "Welcome home."));
						NickServ::UpdateLogin(this);
					}
				} else {
					SendAsServer("436 " + mNickName + " " + nickname + " :" + Utils::make_string(mLang, "Error updating your nick."));
					return;
				}
			} else if (NickServ::Login(nickname, password) == false && password != "") {
				if (NickServ::IsRegistered(nickname) == true && bForce.find(nickname) != bForce.end()) {
					if (bForce.count(nickname) > 0) {
						bForce[nickname]++;
						Send(":" + config->Getvalue("nickserv") + " NOTICE " + mNickName + " :" + Utils::make_string(mLang, "Wrong password."));
						return;
					} else {
						bForce[nickname] = 1;
						Send(":" + config->Getvalue("nickserv") + " NOTICE " + mNickName + " :" + Utils::make_string(mLang, "Wrong password."));
						return;
					}
				}
			}
			else if (Mainframe::instance()->doesNicknameExists(nickname) == true)
			{
				SendAsServer("433 " 
					+ nickname + " " + nickname + " :" + Utils::make_string(mLang, "The nick is used by somebody."));
				return;
			}
			else
			{
				if (Mainframe::instance()->addLocalUser(this, nickname)) {
					mNickName = nickname;
					//if (getMode('w') == false) {
					mCloak = sha256(mHost).substr(0, 16);
					//}
					User::log(Utils::make_string("", "Nickname %s enter to irc with ip: %s", nickname.c_str(), mHost.c_str()));
					Send(":" + nickname + " NICK :"+ nickname);
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
						//SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "There are \002%s\002 connected servers.", std::to_string(Server::count()).c_str()));
						SendAsServer("422 " + mNickName + " :No MOTD");
						if (ssl == true) {
							setMode('z', true);
							SendAsServer("MODE " + nickname + " +z");
						} if (websocket == true) {
							setMode('w', true);
							SendAsServer("MODE " + nickname + " +w");
						} if (OperServ::IsOper(nickname) == true) {
							miRCOps.insert(nickname);
							setMode('o', true);
							SendAsServer("MODE " + nickname + " +o");
						}
						mvHost = mCloak;
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
						Server::sendall("SNICK " + mNickName + " " + mIdent + " " + mHost + " " + std::to_string(bLogin) + " " + mServer + " " + modos);
					}
				} else {
					SendAsServer("436 " + mNickName + " " + nickname + " :" + Utils::make_string(mLang, "Error updating your nick."));
					return;
				}
			}
		}
	} else if (cmd == "JOIN") {
		if (results.size() < 2) {
			SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "More data is needed."));
			return;
		} else if (mNickName == "" || mNickName == "ZeusiRCd") {
			SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You havent used the NICK command yet, you have limited access."));
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
					User::log(Utils::make_string("", "Nick %s joins channel: %s", mNickName.c_str(), chan->mName.c_str()));
					chan->addUser(this);
					mChannels.insert(chan);
					chan->broadcast(messageHeader() + "JOIN :" + chan->mName);
					chan->sendUserList(this);
					Server::sendall("SJOIN " + mNickName + " " + chan->mName + " +x");
				} else {
					Channel *ch = new Channel(this, x[i]);
					if (ch) {
						Mainframe::instance()->addChannel(ch);
						User::log(Utils::make_string("", "Nick %s joins channel: %s", mNickName.c_str(), ch->name().c_str()));
						ch->addUser(this);
						mChannels.insert(ch);
						ch->broadcast(messageHeader() + "JOIN :" + ch->name());
						ch->sendUserList(this);
						Server::sendall("SJOIN " + mNickName + " " + ch->name() + " +x");
					}
				}
			} else {
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
					if (getMode('r') == true) {
						if (ChanServ::IsAKICK(mNickName + "!" + mIdent + "@" + mvHost, x[i]) == true && getMode('o') == false) {
							SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You got AKICK on this channel, you cannot pass."));
							continue;
						}
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
				}
				Channel* chan = Mainframe::instance()->getChannelByName(x[i]);
				if (chan) {
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
						User::log(Utils::make_string("", "Nick %s joins channel: %s", mNickName.c_str(), chan->name().c_str()));
						chan->addUser(this);
						mChannels.insert(chan);
						chan->broadcast(messageHeader() + "JOIN :" + chan->name());
						chan->sendUserList(this);
						Server::sendall("SJOIN " + mNickName + " " + chan->name() + " +x");
						if (ChanServ::IsRegistered(chan->name()) == true) {
							ChanServ::DoRegister(this, chan);
							ChanServ::CheckModes(this, chan->name());
							chan->increaseflood();
						}
					}
				} else {
					chan = new Channel(this, x[i]);
					if (chan) {
						Mainframe::instance()->addChannel(chan);
						User::log(Utils::make_string("", "Nick %s joins channel: %s", mNickName.c_str(), chan->name().c_str()));
						chan->addUser(this);
						mChannels.insert(chan);
						chan->broadcast(messageHeader() + "JOIN :" + chan->name());
						chan->sendUserList(this);
						Server::sendall("SJOIN " + mNickName + " " + chan->name() + " +x");
						if (ChanServ::IsRegistered(chan->name()) == true) {
							ChanServ::DoRegister(this, chan);
							ChanServ::CheckModes(this, chan->name());
							chan->increaseflood();
						}
					}
				}
			}
		}
	} else if (cmd == "PART") {
		if (results.size() < 2) return;
		else if (mNickName == "" || mNickName == "ZeusiRCd") {
			SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You havent used the NICK command yet, you have limited access."));
			return;
		}
		Channel* chan = Mainframe::instance()->getChannelByName(results[1]);
		if (chan) {
			if (chan->hasUser(this) == false) {
				SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You are not into the channel."));
				return;
			}
			User::log(Utils::make_string("", "Nick %s leaves channel: %s", mNickName.c_str(), chan->mName.c_str()));
			chan->broadcast(messageHeader() + "PART " + chan->mName);
			chan->removeUser(this);
			mChannels.erase(chan);
			Server::sendall("SPART " + mNickName + " " + chan->mName);
			chan->increaseflood();
			if (chan->empty())
				Mainframe::instance()->removeChannel(results[1]);
		}
		return;
	} else if (cmd == "WHO") {
		if (results.size() < 2) return;
		else if (mNickName == "" || mNickName == "ZeusiRCd") {
			SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You havent used the NICK command yet, you have limited access."));
			return;
		}
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
	} else if (cmd == "NAMES") {
		if (results.size() < 2) return;
		else if (mNickName == "" || mNickName == "ZeusiRCd") {
			SendAsServer("461 " + mNickName + " :" + Utils::make_string(mLang, "You havent used the NICK command yet, you have limited access."));
			return;
		}
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
}

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
	for (auto channel : mChannels) {
		channel->broadcast_except_me(mNickName, messageHeader() + "PART " + channel->mName + " :vHost");
		Server::sendall("SPART " + mNickName + " " + channel->mName);
		std::string mode = "+";
		if (channel->isOperator(this) == true)
			mode.append("o");
		else if (channel->isHalfOperator(this) == true)
			mode.append("h");
		else if (channel->isVoice(this) == true)
			mode.append("v");
		else
			mode.append("x");
			
		channel->broadcast_except_me(mNickName, messageHeader() + "JOIN :" + channel->mName);
		if (mode != "+x")
			channel->broadcast_except_me(mNickName, ":" + config->Getvalue("chanserv") + " MODE " + channel->mName + " " + mode + " " + mNickName);
		Server::sendall("SJOIN " + mNickName + " " + channel->mName + " " + mode);
	}
	SendAsServer("396 " + mNickName + " " + mCloak + " :is now your hidden host");
}

void LocalUser::Exit() {
	User::log("El nick " + mNickName + " sale del chat");
	std::unique_lock<std::mutex> lock (quit_mtx);
	for (auto channel : mChannels) {
		channel->removeUser(this);
		channel->broadcast(messageHeader() + "QUIT :QUIT");
		if (channel->userCount() == 0)
			Mainframe::instance()->removeChannel(channel->name());
	}
	if (getMode('o') == true)
		miRCOps.erase(mNickName);
	Mainframe::instance()->removeLocalUser(mNickName);
}

int LocalUser::Channs()
{
	return mChannels.size();
}
