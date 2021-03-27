#include "ZeusiRCd.h"
#include "module.h"
#include "Utils.h"
#include "services.h"

class CMD_Mode : public Module
{
	public:
	CMD_Mode() : Module("MODE", 50, false) {};
	~CMD_Mode() {};
	virtual void command(User *user, std::string message) override {
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
			Channel* chan = Channel::GetChannel(results[1]);
			if (ChanServ::IsRegistered(results[1]) == false && user->getMode('o') == false) {
				user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The channel %s is not registered.", results[1].c_str()));
				return;
			} else if (!chan) {
				user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The channel is empty."));
				return;
			} else if (chan->IsOperator(user) == false && chan->IsHalfOperator(user) == false && results.size() != 2 && user->getMode('o') == false) {
				user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "You do not have @ nor %."));
				return;
			} else if (results.size() == 2) {
				std::string sql = "SELECT MODOS from CANALES WHERE NOMBRE='" + results[1] + "';";
				std::string modos = DB::SQLiteReturnString(sql);
				user->SendAsServer("324 " + user->mNickName + " " + results[1] + " " + modos);
				return;
			} else if (results.size() == 3) {
				if (results[2] == "+b" || results[2] == "b") {
					for (auto ban : chan->bans)
						user->SendAsServer("367 " + user->mNickName + " " + results[1] + " " + ban->mask() + " " + ban->whois() + " " + std::to_string(ban->time()));
					user->SendAsServer("368 " + user->mNickName + " " + results[1] + " :" + Utils::make_string(user->mLang, "End of banned."));
				} else if (results[2] == "+B" || results[2] == "B") {
					for (auto ban : chan->pbans)
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
								user->deliver(":" + config["chanserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The BAN already exist."));
							} else if (chan->bans.size() >= config["maxbans"].as<unsigned int>()) {
								user->deliver(":" + config["chanserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "MaxBans has reached."));
							} else {
								chan->setBan(results[3+j], user->mNickName);
								chan->broadcast(user->messageHeader() + "MODE " + chan->name + " +b " + results[3+j]);
								user->deliver(":" + config["chanserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The BAN has been set."));
								Server::Send("CMODE " + user->mNickName + " " + chan->name + " +b " + results[3+j]);
							}
						} else {
							if (chan->IsBan(maskara) == false) {
								user->deliver(":" + config["chanserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The BAN does not exist."));
							} else {
								for (auto ban : chan->bans)
									if (ban->mask() == results[3+j]) {
										chan->UnBan(ban);
										chan->broadcast(user->messageHeader() + "MODE " + chan->name + " -b " + results[3+j]);
										user->deliver(":" + config["chanserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The BAN has been deleted."));
										Server::Send("CMODE " + user->mNickName + " " + chan->name + " -b " + results[3+j]);
										break;
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
								user->deliver(":" + config["chanserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The BAN already exist."));
							} else if (chan->pbans.size() >= config["maxbans"].as<unsigned int>()) {
								user->deliver(":" + config["chanserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "MaxBans has reached."));
							} else {
								chan->setpBan(results[3+j], user->mNickName);
								chan->broadcast(user->messageHeader() + "MODE " + chan->name + " +B " + results[3+j]);
								user->deliver(":" + config["chanserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The BAN has been set."));
								Server::Send("CMODE " + user->mNickName + " " + chan->name + " +B " + results[3+j]);
							}
						} else {
							if (chan->IsBan(maskara) == false) {
								user->deliver(":" + config["chanserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The BAN does not exist."));
							} else {
								for (auto ban : chan->pbans)
									if (ban->mask() == results[3+j]) {
										chan->UnpBan(ban);
										chan->broadcast(user->messageHeader() + "MODE " + chan->name + " -B " + results[3+j]);
										user->deliver(":" + config["chanserv"].as<std::string>() + " NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The BAN has been deleted."));
										Server::Send("CMODE " + user->mNickName + " " + chan->name + " -B " + results[3+j]);
										break;
									}
							}
						}
						j++;
					} else if (results[2][i] == 'o') {
							User *target = User::GetUser(results[3+j]);
							if (target)
							{
								if (chan->HasUser(target) == false) { j++; continue; }
								if (action == 1) {
									if (user->getMode('o') == true && (chan->IsOperator(target) == false)) {
										if (chan->IsVoice(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name + " -v " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name + " -v " + target->mNickName);
											chan->RemoveVoice(target);
										} if (chan->IsHalfOperator(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name + " -h " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name + " -h " + target->mNickName);
											chan->RemoveHalfOperator(target);
										}
										chan->broadcast(user->messageHeader() + "MODE " + chan->name + " +o " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name + " +o " + target->mNickName);
										chan->GiveOperator(target);
									} else if (chan->IsOperator(target) == true) { j++; continue; }
									else if (chan->IsOperator(user) == false) { j++; continue; }
									else {
										if (chan->IsVoice(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name + " -v " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name + " -v " + target->mNickName);
											chan->RemoveVoice(target);
										} if (chan->IsHalfOperator(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name + " -h " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name + " -h " + target->mNickName);
											chan->RemoveHalfOperator(target);
										}
										chan->broadcast(user->messageHeader() + "MODE " + chan->name + " +o " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name + " +o " + target->mNickName);
										chan->GiveOperator(target);
									}
								} else {
									if (chan->IsOperator(user) == true && chan->IsOperator(target) == true) {
										chan->broadcast(user->messageHeader() + "MODE " + chan->name + " -o " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name + " -o " + target->mNickName);
										chan->RemoveOperator(target);
									}
								}
							}
						j++;
					} else if (results[2][i] == 'h') {
							User *target = User::GetUser(results[3+j]);
							if (target)
							{
								if (chan->HasUser(target) == false) { j++; continue; }
								if (action == 1) {
									if (user->getMode('o') == true && chan->IsHalfOperator(target) == false) {
										if (chan->IsOperator(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name + " -o " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name + " -o " + target->mNickName);
											chan->RemoveOperator(target);
										} if (chan->IsVoice(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name + " -v " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name + " -v " + target->mNickName);
											chan->RemoveVoice(target);
										}
										chan->broadcast(user->messageHeader() + "MODE " + chan->name + " +h " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name + " +h " + target->mNickName);
										chan->GiveHalfOperator(target);
									} else if (chan->IsHalfOperator(target) == true) { j++; continue; }
									else if (chan->IsOperator(user) == false) { j++; continue; }
									else {
										if (chan->IsOperator(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name + " -o " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name + " -o " + target->mNickName);
											chan->RemoveOperator(target);
										} if (chan->IsVoice(target) == true) {
											chan->broadcast(user->messageHeader() + "MODE " + chan->name + " -v " + target->mNickName);
											Server::Send("CMODE " + user->mNickName + " " + chan->name + " -v " + target->mNickName);
											chan->RemoveVoice(target);
										}
										chan->broadcast(user->messageHeader() + "MODE " + chan->name + " +h " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name + " +h " + target->mNickName);
										chan->GiveHalfOperator(target);
									}
								} else {
									if (chan->IsHalfOperator(target) == true || chan->IsOperator(user) == true) {
										chan->broadcast(user->messageHeader() + "MODE " + chan->name + " -h " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name + " -h " + target->mNickName);
										chan->RemoveHalfOperator(target);
									}
								}
							}
						j++;
					} else if (results[2][i] == 'v') {
						User *target = User::GetUser(results[3+j]);
						if (target)
						{
							if (chan->HasUser(target) == false) { j++; continue; }
							if (action == 1) {
								if (user->getMode('o') == true && chan->IsVoice(target) == false) {
									if (chan->IsOperator(target) == true) {
										chan->broadcast(user->messageHeader() + "MODE " + chan->name + " -o " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name + " -o " + target->mNickName);
										chan->RemoveOperator(target);
									} if (chan->IsHalfOperator(target) == true) {
										chan->broadcast(user->messageHeader() + "MODE " + chan->name + " -h " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name + " -h " + target->mNickName);
										chan->RemoveHalfOperator(target);
									}
									chan->broadcast(user->messageHeader() + "MODE " + chan->name + " +v " + target->mNickName);
									Server::Send("CMODE " + user->mNickName + " " + chan->name + " +v " + target->mNickName);
									chan->GiveVoice(target);
								} else if (chan->IsVoice(target) == true) { j++; continue; }
								else if (chan->IsOperator(user) == false && chan->IsHalfOperator(user) == false) { j++; continue; }
								else {
									if (chan->IsOperator(target) == true) {
										chan->broadcast(user->messageHeader() + "MODE " + chan->name + " -o " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name + " -o " + target->mNickName);
										chan->RemoveOperator(target);
									} if (chan->IsHalfOperator(target) == true) {
										chan->broadcast(user->messageHeader() + "MODE " + chan->name + " -h " + target->mNickName);
										Server::Send("CMODE " + user->mNickName + " " + chan->name + " -h " + target->mNickName);
										chan->RemoveHalfOperator(target);
									}
									chan->broadcast(user->messageHeader() + "MODE " + chan->name + " +v " + target->mNickName);
									Server::Send("CMODE " + user->mNickName + " " + chan->name + " +v " + target->mNickName);
									chan->GiveVoice(target);
								}
							} else {
								if (chan->IsVoice(target) == true && (chan->IsOperator(user) == true || chan->IsHalfOperator(user) == true)) {
									chan->broadcast(user->messageHeader() + "MODE " + chan->name + " -v " + target->mNickName);
									Server::Send("CMODE " + user->mNickName + " " + chan->name + " -v " + target->mNickName);
									chan->RemoveVoice(target);
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
