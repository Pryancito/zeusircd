#include "ZeusiRCd.h"
#include "module.h"
#include "Server.h"

std::string& ltrim(std::string& str, const std::string& chars = "\t\n\v\f\r ");
std::string& rtrim(std::string& str, const std::string& chars = "\t\n\v\f\r ");
std::string& trim(std::string& str, const std::string& chars = "\t\n\v\f\r ");

class CMD_Kick : public Module
{
	public:
	CMD_Kick() : Module("KICK", 50, false) {};
	~CMD_Kick() {};
	virtual void command(User *user, std::string message) override {
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		if (!user->bSentNick) {
			user->SendAsServer("461 ZeusiRCd :" + Utils::make_string(user->mLang, "You havent used the NICK command yet, you have limited access."));
			return;
		} else if (results.size() < 3) return;
		Channel* chan = Channel::GetChannel(results[1]);
		User*  victim = User::GetUser(results[2]);
		std::string reason = "";
		if (chan && victim) {
			for (unsigned int i = 3; i < results.size(); ++i) {
				reason += results[i] + " ";
			}
			trim(reason);
			if ((chan->IsOperator(user) || chan->IsHalfOperator(user)) && chan->HasUser(victim) && (!chan->IsOperator(victim) || user->getMode('o') == true) && victim->getMode('o') == false) {
				victim->kick(user->mNickName, victim->mNickName, reason, chan);
				Server::Send("SKICK " + user->mNickName + " " + chan->name + " " + victim->mNickName + " " + reason);
			}
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Kick);
}
