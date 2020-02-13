#include "ZeusBaseClass.h"
#include "module.h"
#include "Channel.h"
#include "mainframe.h"
#include "Utils.h"
#include "services.h"

class CMD_Mode : public Module
{
	public:
	CMD_Mode() : Module("MODE", 50, false) {};
	~CMD_Mode() {};
	virtual void command(LocalUser *user, std::string message) override {
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		if (!user->bSentNick) {
			user->SendAsServer("461 ZeusiRCd :" + Utils::make_string(user->mLang, "You havent used the NICK command yet, you have limited access."));
			return;
		}
		else if (results.size() < 2) {
			user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		} else if (results[1][0] == '#') {
			if (Utils::checkchan(results[1]) == false) {
				user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The channel contains no-valid characters."));
				return;
			}
			Channel* chan = Mainframe::instance()->getChannelByName(results[1]);
			if (ChanServ::IsRegistered(results[1]) == false && user->getMode('o') == false) {
				user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The channel %s is not registered.", results[1].c_str()));
				return;
			} else if (!chan) {
				user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The channel is empty."));
				return;
			} else if (chan->isOperator(user) == false && chan->isHalfOperator(user) == false && results.size() != 2 && user->getMode('o') == false) {
				user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "You do not have @ nor %."));
				return;
			} else if (results.size() == 2) {
				std::string sql = "SELECT MODOS from CANALES WHERE NOMBRE='" + results[1] + "';";
				std::string modos = DB::SQLiteReturnString(sql);
				user->SendAsServer("324 " + user->mNickName + " " + results[1] + " " + modos);
				return;
			} else if (results.size() == 3) {
				if (results[2] == "+b" || results[2] == "b") {
					for (auto ban : chan->bans())
						user->SendAsServer("367 " + user->mNickName + " " + results[1] + " " + ban->mask() + " " + ban->whois() + " " + std::to_string(ban->time()));
					user->SendAsServer("368 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "End of banned."));
				} else if (results[2] == "+B" || results[2] == "B") {
					for (auto ban : chan->pbans())
						user->SendAsServer("367 " + user->mNickName + " " + results[1] + " " + ban->mask() + " " + ban->whois() + " " + std::to_string(ban->time()));
					user->SendAsServer("368 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "End of banned."));
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
								user->Send(":" + config["chanserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The BAN already exist."));
							} else {
								chan->setBan(results[3+j], user->mNickName);
								chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " +b " + results[3+j]);
								user->Send(":" + config["chanserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The BAN has been set."));
								Server::Send("CMODE " + user->mNickName + " " + chan->name() + " +b " + results[3+j]);
							}
						} else {
							if (chan->IsBan(maskara) == false) {
								user->Send(":" + config["chanserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The BAN does not exist."));
							} else {
								BanSet bans = chan->bans();
								BanSet::iterator it = bans.begin();
								for (; it != bans.end(); ++it)
									if ((*it)->mask() == results[3+j]) {
										chan->UnBan((*it));
										chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -b " + results[3+j]);
										user->Send(":" + config["chanserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The BAN has been deleted."));
										Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -b " + results[3+j]);
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
								user->Send(":" + config["chanserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The BAN already exist."));
							} else {
								chan->setpBan(results[3+j], user->mNickName);
								chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " +B " + results[3+j]);
								user->Send(":" + config["chanserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The BAN has been set."));
								Server::Send("CMODE " + user->mNickName + " " + chan->name() + " +B " + results[3+j]);
							}
						} else {
							if (chan->IsBan(maskara) == false) {
								user->Send(":" + config["chanserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The BAN does not exist."));
							} else {
								pBanSet bans = chan->pbans();
								pBanSet::iterator it = bans.begin();
								for (; it != bans.end(); ++it)
									if ((*it)->mask() == results[3+j]) {
										chan->UnpBan((*it));
										chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -B " + results[3+j]);
										user->Send(":" + config["chanserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The BAN has been deleted."));
										Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -B " + results[3+j]);
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
									if (user->getMode('o') == true && (chan->isOperator(target) == false)) {
										if (chan->isVoice(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -v " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -v " + target->mNickName);
											chan->delVoice(target);
										} if (chan->isHalfOperator(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -h " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -h " + target->mNickName);
											chan->delHalfOperator(target);
										}
										chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " +o " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name() + " +o " + target->mNickName);
										chan->giveOperator(target);
									} else if (chan->isOperator(target) == true) { j++; continue; }
									else if (chan->isOperator(user) == false) { j++; continue; }
									else {
										if (chan->isVoice(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -v " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -v " + target->mNickName);
											chan->delVoice(target);
										} if (chan->isHalfOperator(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -h " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -h " + target->mNickName);
											chan->delHalfOperator(target);
										}
										chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " +o " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name() + " +o " + target->mNickName);
										chan->giveOperator(target);
									}
								} else {
									if (chan->isOperator(user) == true && chan->isOperator(target) == true) {
										chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -o " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -o " + target->mNickName);
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
									if (user->getMode('o') == true && (chan->isOperator(target) == false)) {
										if (chan->isVoice(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -v " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -v " + target->mNickName);
											chan->delVoice(target);
										} if (chan->isHalfOperator(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -h " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -h " + target->mNickName);
											chan->delHalfOperator(target);
										}
										chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " +o " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name() + " +o " + target->mNickName);
										chan->giveOperator(target);
									} else if (chan->isOperator(target) == true) { j++; continue; }
									else if (chan->isOperator(user) == false) { j++; continue; }
									else {
										if (chan->isVoice(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -v " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -v " + target->mNickName);
											chan->delVoice(target);
										} if (chan->isHalfOperator(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -h " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -h " + target->mNickName);
											chan->delHalfOperator(target);
										}
										chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " +o " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name() + " +o " + target->mNickName);
										chan->giveOperator(target);
									}
								} else {
									if (chan->isOperator(user) == true && chan->isOperator(target) == true) {
										chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -o " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -o " + target->mNickName);
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
									if (user->getMode('o') == true && chan->isHalfOperator(target) == false) {
										if (chan->isOperator(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -o " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -o " + target->mNickName);
											chan->delOperator(target);
										} if (chan->isVoice(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -v " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -v " + target->mNickName);
											chan->delVoice(target);
										}
										chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " +h " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name() + " +h " + target->mNickName);
										chan->giveHalfOperator(target);
									} else if (chan->isHalfOperator(target) == true) { j++; continue; }
									else if (chan->isOperator(user) == false) { j++; continue; }
									else {
										if (chan->isOperator(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -o " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -o " + target->mNickName);
											chan->delOperator(target);
										} if (chan->isVoice(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -v " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -v " + target->mNickName);
											chan->delVoice(target);
										}
										chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " +h " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name() + " +h " + target->mNickName);
										chan->giveHalfOperator(target);
									}
								} else {
									if (chan->isHalfOperator(target) == true || chan->isOperator(user) == true) {
										chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -h " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -h " + target->mNickName);
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
									if (user->getMode('o') == true && chan->isHalfOperator(target) == false) {
										if (chan->isOperator(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -o " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -o " + target->mNickName);
											chan->delOperator(target);
										} if (chan->isVoice(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -v " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -v " + target->mNickName);
											chan->delVoice(target);
										}
										chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " +h " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name() + " +h " + target->mNickName);
										chan->giveHalfOperator(target);
									} else if (chan->isHalfOperator(target) == true) { j++; continue; }
									else if (chan->isOperator(user) == false) { j++; continue; }
									else {
										if (chan->isOperator(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -o " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -o " + target->mNickName);
											chan->delOperator(target);
										} if (chan->isVoice(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -v " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -v " + target->mNickName);
											chan->delVoice(target);
										}
										chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " +h " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name() + " +h " + target->mNickName);
										chan->giveHalfOperator(target);
									}
								} else {
									if (chan->isHalfOperator(target) == true || chan->isOperator(user) == true) {
										chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -h " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -h " + target->mNickName);
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
									if (user->getMode('o') == true && chan->isVoice(target) == false) {
										if (chan->isOperator(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -o " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -o " + target->mNickName);
											chan->delOperator(target);
										} if (chan->isHalfOperator(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -h " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -h " + target->mNickName);
											chan->delHalfOperator(target);
										}
										chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " +v " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name() + " +v " + target->mNickName);
										chan->giveVoice(target);
									} else if (chan->isVoice(target) == true) { j++; continue; }
									else if (chan->isOperator(user) == false && chan->isHalfOperator(user) == false) { j++; continue; }
									else {
										if (chan->isOperator(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -o " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -o " + target->mNickName);
											chan->delOperator(target);
										} if (chan->isHalfOperator(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -h " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -h " + target->mNickName);
											chan->delHalfOperator(target);
										}
										chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " +v " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name() + " +v " + target->mNickName);
										chan->giveVoice(target);
									}
								} else {
									if (chan->isVoice(target) == true && (chan->isOperator(user) == true || chan->isHalfOperator(user) == true)) {
										chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -v " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -v " + target->mNickName);
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
									if (user->getMode('o') == true && chan->isVoice(target) == false) {
										if (chan->isOperator(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -o " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -o " + target->mNickName);
											chan->delOperator(target);
										} if (chan->isHalfOperator(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -h " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -h " + target->mNickName);
											chan->delHalfOperator(target);
										}
										chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " +v " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name() + " +v " + target->mNickName);
										chan->giveVoice(target);
									} else if (chan->isVoice(target) == true) { j++; continue; }
									else if (chan->isOperator(user) == false && chan->isHalfOperator(user) == false) { j++; continue; }
									else {
										if (chan->isOperator(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -o " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -o " + target->mNickName);
											chan->delOperator(target);
										} if (chan->isHalfOperator(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -h " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -h " + target->mNickName);
											chan->delHalfOperator(target);
										}
										chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " +v " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name() + " +v " + target->mNickName);
										chan->giveVoice(target);
									}
								} else {
									if (chan->isVoice(target) == true && (chan->isOperator(user) == true || chan->isHalfOperator(user) == true)) {
										chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -v " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name() + " -v " + target->mNickName);
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
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Mode);
}
