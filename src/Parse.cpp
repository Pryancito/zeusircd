#include "ZeusBaseClass.h"
#include "Config.h"
#include "Timer.h"
#include "Utils.h"
#include "services.h"
#include "oper.h"
#include "sha256.h"
#include "Server.h"
#include "Channel.h"

#include <string>
#include <iterator>
#include <algorithm>
#include <mutex>

std::mutex log_mtx;
TimerWheel timers;
extern time_t encendido;
extern OperSet miRCOps;
std::map<std::string, LocalUser*> LocalUsers;
std::map<std::string, RemoteUser*> RemoteUsers;
std::map<std::string, unsigned int> bForce;
extern std::map<std::string, Channel*> Channels;

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
	std::istringstream iss(message);
	std::vector<std::string> results(std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>());
	
	if (results.size() == 0)
		return;
	
	std::string cmd = results[0];
	std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
	
	if (cmd == "QUIT") {
		quit = true;
		Close();
	} else if (cmd == "PING") {
		bPing = time(0);
		Send(":" + config->Getvalue("serverName") + " PONG " + config->Getvalue("serverName") + (results.size() > 2 ? results[1] : ""));
	} else if (cmd == "PONG") {
		bPing = time(0);
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
				if (User::FindUser(nickname) == true)
				{
					LocalUser *user = LocalUser::FindLocalUser(nickname);
					if (user != nullptr)
					{
						user->quit = true;
						user->Close();
					} else {
						RemoteUser *user = RemoteUser::FindRemoteUser(nickname);
						if (user != nullptr)
						{
							
						}
					}
				}
				if (getMode('r') == false) {
					setMode('r', true);
					SendAsServer("MODE " + nickname + " +r");
					Server::sendall("UMODE " + nickname + " +r");
				}
				std::string lang = NickServ::GetLang(nickname);
				if (lang != "")
					mLang = lang;
				if (LocalUser::addUser(this, nickname)) {
					mNickName = nickname;
					if (getMode('w') == false) {
						mCloak = sha256(mHost).substr(0, 16);
					}
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
						SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "There are \002%s\002 users and \002%s\002 channels.", std::to_string(LocalUsers.size() + RemoteUsers.size()).c_str(), std::to_string(Channels.size()).c_str()));
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
						std::string cloak = NickServ::GetvHost(nickname);
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
			} else if (NickServ::IsRegistered(nickname) == true && bForce.find(nickname) != bForce.end()) {
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
			else if (User::FindUser(nickname) == true)
			{
				SendAsServer("433 " 
					+ nickname + " " + nickname + " :" + Utils::make_string(mLang, "The nick is used by somebody."));
				return;
			}
			else
			{
				if (LocalUser::addUser(this, nickname)) {
					mNickName = nickname;
					if (getMode('w') == false) {
						mCloak = sha256(mHost).substr(0, 16);
					}
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
						SendAsServer("002 " + mNickName + " :" + Utils::make_string(mLang, "There are \002%s\002 users and \002%s\002 channels.", std::to_string(LocalUsers.size() + RemoteUsers.size()).c_str(), std::to_string(Channels.size()).c_str()));
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
		Send("PING :" + config->Getvalue("serverName"));
		StartTimers(&timers);
	}
};

void LocalUser::StartTimers(TimerWheel* timers)
{
	if (!bSentNick)
		timers->schedule(&tnick, 10);
	timers->schedule(&tping, 60);
}

void LocalUser::SendAsServer(const std::string message)
{
	Send(":"+config->Getvalue("serverName")+" "+message);
}

bool User::FindUser(std::string nick)
{
	std::transform(nick.begin(), nick.end(), nick.begin(), ::tolower);
	return (LocalUsers.find(nick) != LocalUsers.end() || RemoteUsers.find(nick) != RemoteUsers.end());
}

LocalUser *LocalUser::FindLocalUser(std::string nick)
{
	std::transform(nick.begin(), nick.end(), nick.begin(), ::tolower);
	if (!User::FindUser(nick))
		return nullptr;
	else
		return LocalUsers[nick];
}

RemoteUser *RemoteUser::FindRemoteUser(std::string nick)
{
	std::transform(nick.begin(), nick.end(), nick.begin(), ::tolower);
	if (!User::FindUser(nick))
		return nullptr;
	else
		return RemoteUsers[nick];
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

bool LocalUser::addUser(LocalUser* user, std::string nick) {
	std::transform(nick.begin(), nick.end(), nick.begin(), ::tolower);
    if(FindUser(nick) == true) return false;
    LocalUsers[nick] = user;
    return true;
}

bool LocalUser::changeNickname(std::string old, std::string recent) {
	std::transform(recent.begin(), recent.end(), recent.begin(), ::tolower);
	std::transform(old.begin(), old.end(), old.begin(), ::tolower);
    LocalUser* tmp = LocalUsers[old];
    LocalUsers.erase(old);
    LocalUsers[recent] = tmp;
    return true;
}

void LocalUser::removeUser(std::string nick) {
	if(FindUser(nick) == false) return;
	std::transform(nick.begin(), nick.end(), nick.begin(), ::tolower);
	auto it = LocalUsers.find(nick);
	it->second = nullptr;
	delete it->second;
	LocalUsers.erase (nick);
}

bool RemoteUser::addUser(RemoteUser* user, std::string nick) {
	std::transform(nick.begin(), nick.end(), nick.begin(), ::tolower);
    if(FindUser(nick) == true) return false;
    RemoteUsers[nick] = user;
    return true;
}

bool RemoteUser::changeNickname(std::string old, std::string recent) {
	std::transform(recent.begin(), recent.end(), recent.begin(), ::tolower);
	std::transform(old.begin(), old.end(), old.begin(), ::tolower);
    RemoteUser* tmp = RemoteUsers[old];
    RemoteUsers.erase(old);
    RemoteUsers[recent] = tmp;
    return true;
}

void RemoteUser::removeUser(std::string nick) {
	if(FindUser(nick) == false) return;
	std::transform(nick.begin(), nick.end(), nick.begin(), ::tolower);
	auto it = RemoteUsers.find(nick);
	it->second = nullptr;
	delete it->second;
	RemoteUsers.erase (nick);
}

int LocalUser::Channs() {
	return mChannels.size();
}
