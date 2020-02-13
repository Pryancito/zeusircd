#include "ZeusBaseClass.h"
#include "module.h"
#include "Channel.h"
#include "mainframe.h"

class CMD_Who : public Module
{
	public:
	CMD_Who() : Module("WHO", 50, false) {};
	~CMD_Who() {};
	virtual void command(LocalUser *user, std::string message) override {
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		if (!user->bSentNick) {
			user->SendAsServer("461 ZeusiRCd :" + Utils::make_string(user->mLang, "You havent used the NICK command yet, you have limited access."));
			return;
		} else if (results.size() < 2) return;
		Channel* chan = Mainframe::instance()->getChannelByName(results[1]);
		if (!chan) {
			user->SendAsServer("403 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The channel doesnt exists."));
			return;
		} else if (!chan->hasUser(user)) {
			user->SendAsServer("441 " + user->mNickName + " :" + Utils::make_string(user->mLang, "You are not into the channel."));
			return;
		} else {
			chan->sendWhoList(user);
			return;
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Who);
}
