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
#include "services.h"

class CMD_Whois : public Module
{
	public:
	CMD_Whois() : Module("WHOIS", 50, false) {};
	~CMD_Whois() {};
	virtual void command(User *user, std::string message) override {
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		if (!user->bSentNick) {
			user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "You havent used the NICK command yet, you have limited access."));
			return;
		} else if (results.size() < 2) {
			user->SendAsServer("431 " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		} else if (results[1][0] == '#') {
			if (Utils::checkchan (results[1]) == false) {
				user->SendAsServer("401 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "The channel contains no-valid characters."));
				user->SendAsServer("318 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "End of /WHOIS."));
				return;
			}
			Channel* chan = Channel::GetChannel(results[1]);
			std::string sql;
			std::string mascara = user->mNickName + "!" + user->mIdent + "@" + user->mCloak;
			if (ChanServ::IsAKICK(mascara, results[1]) == true) {
				user->SendAsServer("320 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "STATUS: \0036AKICK\003."));
			} else if (chan && chan->IsBan(mascara) == true)
				user->SendAsServer("320 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "STATUS: \0036BANNED\003."));
			if (user->getMode('r') == true && !NickServ::GetvHost(user->mNickName).empty()) {
				mascara = user->mNickName + "!" + user->mIdent + "@" + user->mvHost;
				if (ChanServ::IsAKICK(mascara, results[1]) == true) {
					user->SendAsServer("320 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "STATUS: \0036AKICK\003."));
				} else if (chan && chan->IsBan(mascara) == true)
					user->SendAsServer("320 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "STATUS: \0036BANNED\003."));
			} if (!chan)
				user->SendAsServer("320 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "STATUS: \0034EMPTY\003."));
			else
				user->SendAsServer("320 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "STATUS: \0033ACTIVE\003."));
			if (ChanServ::IsRegistered(results[1]) == 1) {
				user->SendAsServer("320 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "The channel is registered."));
				switch (ChanServ::Access(user->mNickName, results[1])) {
					case 0:
						user->SendAsServer("320 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "You do not have access."));
						break;
					case 1:
						user->SendAsServer("320 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "Your access is VOP."));
						break;
					case 2:
						user->SendAsServer("320 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "Your access is HOP."));
						break;
					case 3:
						user->SendAsServer("320 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "Your access is AOP."));
						break;
					case 4:
						user->SendAsServer("320 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "Your access is SOP."));
						break;
					case 5:
						user->SendAsServer("320 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "Your access is FOUNDER."));
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
					user->SendAsServer("320 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "The channel has no modes."));
				else
					user->SendAsServer("320 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "The channel has modes:%s", modes.c_str()));
				if (ChanServ::IsKEY(results[1]) == 1 && ChanServ::Access(user->mNickName, results[1]) != 0) {
					sql = "SELECT CLAVE FROM CANALES WHERE NOMBRE='" + results[1] + "';";
					std::string key = DB::SQLiteReturnString(sql);
					if (key.length() > 0)
						user->SendAsServer("320 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "The channel key is: %s", key.c_str()));
				}
				std::string sql = "SELECT OWNER FROM CANALES WHERE NOMBRE='" + results[1] + "';";
				std::string owner = DB::SQLiteReturnString(sql);
				if (owner.length() > 0)
					user->SendAsServer("320 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "The founder of the channel is: %s", owner.c_str()));
				sql = "SELECT REGISTERED FROM CANALES WHERE NOMBRE='" + results[1] + "';";
				int registro = DB::SQLiteReturnInt(sql);
				if (registro > 0) {
					std::string tiempo = Utils::Time(registro);
					user->SendAsServer("320 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "Registered for: %s", tiempo.c_str()));
				}
				user->SendAsServer("318 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "End of /WHOIS."));
			} else {
				user->SendAsServer("401 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "The channel is not registered."));
				user->SendAsServer("318 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "End of /WHOIS."));
			}
		} else {
			if (Utils::checknick (results[1]) == false) {
				user->SendAsServer("401 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "The nick contains no-valid characters."));
				user->SendAsServer("318 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "End of /WHOIS."));
				return;
			}
			User *target = User::GetUser(results[1]);
			std::string sql;
			if (!target && NickServ::IsRegistered(results[1]) == true) {
				user->SendAsServer("320 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "STATUS: \0034OFFLINE\003."));
				user->SendAsServer("320 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "The nick is registered."));
				if (OperServ::IsOper(results[1]) == true)
					user->SendAsServer("320 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "Is an iRCop."));
				sql = "SELECT SHOWMAIL FROM OPTIONS WHERE NICKNAME='" + results[1] + "';";
				if (DB::SQLiteReturnInt(sql) == 1 || user->getMode('o') == true) {
					sql = "SELECT EMAIL FROM NICKS WHERE NICKNAME='" + results[1] + "';";
					std::string email = DB::SQLiteReturnString(sql);
					if (email.length() > 0)
					user->SendAsServer("320 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "The email is: %s", email.c_str()));
				}
				sql = "SELECT URL FROM NICKS WHERE NICKNAME='" + results[1] + "';";
				std::string url = DB::SQLiteReturnString(sql);
				if (url.length() > 0)
					user->SendAsServer("320 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "The website is: %s", url.c_str()));
				sql = "SELECT VHOST FROM NICKS WHERE NICKNAME='" + results[1] + "';";
				std::string vHost = DB::SQLiteReturnString(sql);
				if (vHost.length() > 0)
					user->SendAsServer("320 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "The vHost is: %s", vHost.c_str()));
				sql = "SELECT REGISTERED FROM NICKS WHERE NICKNAME='" + results[1] + "';";
				int registro = DB::SQLiteReturnInt(sql);
				if (registro > 0) {
					std::string tiempo = Utils::Time(registro);
					user->SendAsServer("320 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "Registered for: %s", tiempo.c_str()));
				}
				sql = "SELECT LASTUSED FROM NICKS WHERE NICKNAME='" + results[1] + "';";
				int last = DB::SQLiteReturnInt(sql);
				std::string tiempo = Utils::Time(last);
				if (tiempo.length() > 0 && last > 0)
					user->SendAsServer("320 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "Last seen on: %s", tiempo.c_str()));
				user->SendAsServer("318 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "End of /WHOIS."));
				return;
			} else if (target && NickServ::IsRegistered(results[1]) == 1) {
				user->SendAsServer("320 " + user->mNickName + " " + target->mNickName + " :" + Utils::make_string(user->mLang, "%s is: %s!%s@%s", target->mNickName.c_str(), target->mNickName.c_str(), target->mIdent.c_str(), target->mCloak.c_str()));
				user->SendAsServer("320 " + user->mNickName + " " + target->mNickName + " :" + Utils::make_string(user->mLang, "STATUS: \0033CONNECTED\003."));
				user->SendAsServer("320 " + user->mNickName + " " + target->mNickName + " :" + Utils::make_string(user->mLang, "The nick is registered."));
				if (user->getMode('o') == true || target->mNickName == user->mNickName) {
					user->SendAsServer("320 " + user->mNickName + " " + target->mNickName + " :" + Utils::make_string(user->mLang, "The IP is: %s", target->mHost.c_str()));
					user->SendAsServer("320 " + user->mNickName + " " + target->mNickName + " :" + Utils::make_string(user->mLang, "The server is: %s", target->mServer.c_str()));
				}
				if (target->getMode('o') == true)
					user->SendAsServer("320 " + user->mNickName + " " + target->mNickName + " :" + Utils::make_string(user->mLang, "Is an iRCop."));
				if (target->getMode('z') == true)
					user->SendAsServer("320 " + user->mNickName + " " + target->mNickName + " :" + Utils::make_string(user->mLang, "Connects trough a secure channel SSL."));
				if (target->getMode('w') == true)
					user->SendAsServer("320 " + user->mNickName + " " + target->mNickName + " :" + Utils::make_string(user->mLang, "Connects trough WebChat."));
				if (NickServ::GetOption("SHOWMAIL", target->mNickName) == true || user->getMode('o') == true) {
					sql = "SELECT EMAIL FROM NICKS WHERE NICKNAME='" + target->mNickName + "';";
					std::string email = DB::SQLiteReturnString(sql);
					if (email.length() > 0)
					user->SendAsServer("320 " + user->mNickName + " " + target->mNickName + " :" + Utils::make_string(user->mLang, "The email is: %s", email.c_str()));
				}
				sql = "SELECT URL FROM NICKS WHERE NICKNAME='" + target->mNickName + "';";
				std::string url = DB::SQLiteReturnString(sql);
				if (url.length() > 0)
					user->SendAsServer("320 " + user->mNickName + " " + target->mNickName + " :" + Utils::make_string(user->mLang, "The website is: %s", url.c_str()));
				sql = "SELECT VHOST FROM NICKS WHERE NICKNAME='" + target->mNickName + "';";
				std::string vHost = DB::SQLiteReturnString(sql);
				if (vHost.length() > 0)
					user->SendAsServer("320 " + user->mNickName + " " + target->mNickName + " :" + Utils::make_string(user->mLang, "The vHost is: %s", vHost.c_str()));
				user->SendAsServer("320 " + user->mNickName + " " + target->mNickName + " :" + Utils::make_string(user->mLang, "Country: %s", Utils::GetEmoji(target->mHost).c_str()));
				if (target->bAway == true)
					user->SendAsServer("320 " + user->mNickName + " " + target->mNickName + " :AWAY " + target->mAway);
				if (user->mNickName == target->mNickName && user->getMode('r') == true) {
					std::string opciones;
					if (NickServ::GetOption("NOACCESS", user->mNickName) == 1) {
						if (!opciones.empty())
							opciones.append(", ");
						opciones.append("NOACCESS");
					}
					if (NickServ::GetOption("SHOWMAIL", user->mNickName) == 1) {
						if (!opciones.empty())
							opciones.append(", ");
						opciones.append("SHOWMAIL");
					}
					if (NickServ::GetOption("NOMEMO", user->mNickName) == 1) {
						if (!opciones.empty())
							opciones.append(", ");
						opciones.append("NOMEMO");
					}
					if (NickServ::GetOption("NOOP", user->mNickName) == 1) {
						if (!opciones.empty())
							opciones.append(", ");
						opciones.append("NOOP");
					}
					if (NickServ::GetOption("ONLYREG", user->mNickName) == 1) {
						if (!opciones.empty())
							opciones.append(", ");
						opciones.append("ONLYREG");
					}
					if (NickServ::GetOption("NOCOLOR", user->mNickName) == 1) {
						if (!opciones.empty())
							opciones.append(", ");
						opciones.append("NOCOLOR");
					}
					if (opciones.length() == 0)
						opciones = Utils::make_string(user->mLang, "None");
					user->SendAsServer("320 " + user->mNickName + " " + target->mNickName + " :" + Utils::make_string(user->mLang, "Your options are: %s", opciones.c_str()));
				}
				sql = "SELECT REGISTERED FROM NICKS WHERE NICKNAME='" + target->mNickName + "';";
				int registro = DB::SQLiteReturnInt(sql);
				if (registro > 0) {
					std::string tiempo = Utils::Time(registro);
					user->SendAsServer("320 " + user->mNickName + " " + target->mNickName + " :" + Utils::make_string(user->mLang, "Registered for: %s", tiempo.c_str()));
				}
				std::string tiempo = Utils::Time(target->bLogin);
				if (tiempo.length() > 0)
					user->SendAsServer("320 " + user->mNickName + " " + target->mNickName + " :" + Utils::make_string(user->mLang, "Connected from: %s", tiempo.c_str()));
				user->SendAsServer("318 " + user->mNickName + " " + target->mNickName + " :" + Utils::make_string(user->mLang, "End of /WHOIS."));
				return;
			} else if (target && NickServ::IsRegistered(results[1]) == 0) {
				user->SendAsServer("320 " + user->mNickName + " " + target->mNickName + " :" + Utils::make_string(user->mLang, "%s is: %s!%s@%s", target->mNickName.c_str(), target->mNickName.c_str(), target->mIdent.c_str(), target->mCloak.c_str()));
				user->SendAsServer("320 " + user->mNickName + " " + target->mNickName + " :" + Utils::make_string(user->mLang, "STATUS: \0033CONNECTED\003."));
				if (user->getMode('o') == true) {
					user->SendAsServer("320 " + user->mNickName + " " + target->mNickName + " :" + Utils::make_string(user->mLang, "The IP is: %s", target->mHost.c_str()));
					user->SendAsServer("320 " + user->mNickName + " " + target->mNickName + " :" + Utils::make_string(user->mLang, "The server is: %s", target->mServer.c_str()));
				}
				if (target->getMode('o') == true)
					user->SendAsServer("320 " + user->mNickName + " " + target->mNickName + " :" + Utils::make_string(user->mLang, "Is an iRCop."));
				if (target->getMode('z') == true)
					user->SendAsServer("320 " + user->mNickName + " " + target->mNickName + " :" + Utils::make_string(user->mLang, "Connects trough a secure channel SSL."));
				if (target->getMode('w') == true)
					user->SendAsServer("320 " + user->mNickName + " " + target->mNickName + " :" + Utils::make_string(user->mLang, "Connects trough WebChat."));
				user->SendAsServer("320 " + user->mNickName + " " + target->mNickName + " :" + Utils::make_string(user->mLang, "Country: %s", Utils::GetEmoji(target->mHost).c_str()));
				if (target->bAway == true)
					user->SendAsServer("320 " + user->mNickName + " " + target->mNickName + " :AWAY " + target->mAway);
				std::string tiempo = Utils::Time(target->bLogin);
				if (tiempo.length() > 0)
					user->SendAsServer("320 " + user->mNickName + " " + target->mNickName + " :" + Utils::make_string(user->mLang, "Connected from: %s", tiempo.c_str()));
				user->SendAsServer("318 " + user->mNickName + " " + target->mNickName + " :" + Utils::make_string(user->mLang, "End of /WHOIS."));
				return;
			} else {
				user->SendAsServer("401 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "The nick does not exist."));
				user->SendAsServer("318 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "End of /WHOIS."));
				return;
			}
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Whois);
}
