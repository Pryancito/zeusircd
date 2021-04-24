#include "ZeusiRCd.h"
#include "module.h"

class CMD_Names : public Module
{
	public:
	CMD_Names() : Module("NAMES", 50, false) {};
	~CMD_Names() {};
	virtual void command(User *user, std::string message) override {
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		if (!user->bSentNick) {
			user->SendAsServer("461 ZeusiRCd :" + Utils::make_string(user->mLang, "You havent used the NICK command yet, you have limited access."));
			return;
		}
		else if (results.size() < 2) return;
		Channel* chan = Channel::GetChannel(results[1]);
		if (!chan) {
			user->SendAsServer("403 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The channel doesnt exists."));
			return;
		} else if (!chan->HasUser(user)) {
			user->SendAsServer("441 " + user->mNickName + " :" + Utils::make_string(user->mLang, "You are not into the channel."));
			return;
		} else {
			chan->send_userlist(user);
			return;
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Names);
}
