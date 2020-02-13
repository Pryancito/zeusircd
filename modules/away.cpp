#include "ZeusBaseClass.h"
#include "module.h"
#include "Utils.h"

std::string& ltrim(std::string& str, const std::string& chars = "\t\n\v\f\r ");
std::string& rtrim(std::string& str, const std::string& chars = "\t\n\v\f\r ");
std::string& trim(std::string& str, const std::string& chars = "\t\n\v\f\r ");

class CMD_Away : public Module
{
	public:
	CMD_Away() : Module("AWAY", 50, false) {};
	~CMD_Away() {};
	virtual void command(LocalUser *user, std::string message) override {
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		if (!user->bSentNick) {
			user->SendAsServer("461 ZeusiRCd :" + Utils::make_string(user->mLang, "You havent used the NICK command yet, you have limited access."));
			return;
		} else if (results.size() == 1) {
			user->cmdAway("", false);
			user->SendAsServer("305 " + user->mNickName + " :AWAY OFF");
			return;
		} else {
			std::string away = "";
			for (unsigned int i = 1; i < results.size(); ++i) { away.append(results[i] + " "); }
			trim(away);
			user->cmdAway(away, true);
			user->SendAsServer("306 " + user->mNickName + " :AWAY ON " + away);
			return;
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Away);
}
