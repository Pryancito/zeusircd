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
#include "Utils.h"
#include "Server.h"
#include "services.h"

class CMD_Join : public Module
{
	public:
	CMD_Join() : Module("JOIN", 50, false) {};
	~CMD_Join() {};
	virtual void command(User *user, std::string message) override {
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		if (!user->bSentNick) {
			user->SendAsServer("461 ZeusiRCd :" + Utils::make_string(user->mLang, "You havent used the NICK command yet, you have limited access."));
			return;
		}
		if (results.size() < 2) {
			user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		}
		std::vector<std::string>  x;
		int j = 2;
		Utils::split(results[1], x, ",");
		for (unsigned int i = 0; i < x.size(); i++) {
			if (Utils::checkchan(x[i]) == false) {
				user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The channel contains no-valid characters."));
				continue;
			} else if (x[i].size() < 2) {
				user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The channel name is empty."));
				continue;
			} else if (x[i].length() > config["chanlen"].as<unsigned int>()) {
				user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The channel name is too long."));
				continue;
			} else if ((int) user->channels.size() >= config["maxchannels"].as<int>() && OperServ::IsException(user->mHost, "channel") == 0) {
				user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "You enter in too much channels."));
				continue;
			} else if (OperServ::IsException(user->mHost, "channel") > 0 && (int) user->channels.size() >= OperServ::IsException(user->mHost, "channel")) {
				user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "You enter in too much channels."));
				continue;
			} else if (ChanServ::IsRegistered(x[i]) == false) {
				Channel* chan = Channel::GetChannel(x[i]);
				if (chan) {
					if (chan->HasUser(user) == true) {
						user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "You are already on this channel."));
						continue;
					}
					chan->join(user);
					Server::Send("SJOIN " + user->mNickName + " " + chan->name + " +x");
					continue;
				} else {
					Channel *chan = new Channel(x[i]);
					if (chan) {
						std::transform(x[i].begin(), x[i].end(), x[i].begin(), ::tolower);
						Channels.insert(std::pair<std::string,Channel *>(x[i],chan));
						chan->GiveOperator(user);
						chan->join(user);
						Server::Send("SJOIN " + user->mNickName + " " + chan->name + " +o");
						continue;
					}
				}
			} else {
				Channel* chan = Channel::GetChannel(x[i]);
				if (ChanServ::HasMode(x[i], "ONLYREG") == true && user->getMode('r') == false && user->getMode('o') == false) {
					user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The entrance is only allowed to registered nicks."));
					continue;
				} else if (ChanServ::HasMode(x[i], "ONLYSECURE") == true && user->getMode('z') == false && user->getMode('o') == false) {
					user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The entrance is only allowed to SSL users."));
					continue;
				} else if (ChanServ::HasMode(x[i], "ONLYWEB") == true && user->getMode('w') == false && user->getMode('o') == false) {
					user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The entrance is only allowed to WebChat users."));
					continue;
				} else if (ChanServ::HasMode(x[i], "ONLYACCESS") == true && ChanServ::Access(user->mNickName, x[i]) == 0 && user->getMode('o') == false) {
					user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The entrance is only allowed to accessed users."));
					continue;
				} else if (ChanServ::HasMode(x[i], "COUNTRY") == true && ChanServ::CanGeoIP(user, x[i]) == false && user->getMode('o') == false) {
					user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The entrance is not allowed to users from your country."));
					continue;
				} else {
					if (ChanServ::IsAKICK(user->mNickName + "!" + user->mIdent + "@" + user->mCloak, x[i]) == true && user->getMode('o') == false) {
						user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "You got AKICK on this channel, you cannot pass."));
						continue;
					}
					if (ChanServ::IsAKICK(user->mNickName + "!" + user->mIdent + "@" + user->mvHost, x[i]) == true && user->getMode('o') == false) {
						user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "You got AKICK on this channel, you cannot pass."));
						continue;
					}
					if (ChanServ::IsKEY(x[i]) == true && user->getMode('o') == false) {
						if (results.size() < 3) {
							user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "I need more data: [ /join #channel password ]"));
							continue;
						} else if (ChanServ::CheckKEY(x[i], results[j]) == false) {
							user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "Wrong password."));
							continue;
						} else
							j++;
					}
				} if (chan) {
					if (chan->HasUser(user) == true) {
						user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "You are already on this channel."));
						continue;
					} else if (chan->IsBan(user->mNickName + "!" + user->mIdent + "@" + user->mCloak) == true && user->getMode('o') == false) {
						user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "You are banned, cannot pass."));
						continue;
					} else if (chan->IsBan(user->mNickName + "!" + user->mIdent + "@" + user->mvHost) == true && user->getMode('o') == false) {
						user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "You are banned, cannot pass."));
						continue;
					} else if (chan->isonflood() == true && user->getMode('o') == false) {
						user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The channel is on flood, you cannot pass."));
						continue;
					} else {
						chan->join(user);
						ChanServ::DoRegister(user, chan);
						ChanServ::CheckModes(user, chan->name);
						chan->increaseflood();
						continue;
					}
				} else {
					chan = new Channel(x[i]);
					if (chan) {
						std::transform(x[i].begin(), x[i].end(), x[i].begin(), ::tolower);
						Channels.insert(std::pair<std::string,Channel *>(x[i],chan));
						chan->join(user);
						ChanServ::DoRegister(user, chan);
						ChanServ::CheckModes(user, chan->name);
						chan->increaseflood();
						continue;
					}
				}
			}
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Join);
}
