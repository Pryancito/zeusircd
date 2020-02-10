#include "ZeusBaseClass.h"
#include "module.h"
#include "mainframe.h"
#include "Channel.h"

class CMD_Part : public Module
{
	public:
	CMD_Part() : Module("PART", 50, false) {};
	~CMD_Part() {};
	virtual void command(LocalUser *user, std::string message) override {
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		if (!user->bSentNick) {
			user->SendAsServer("461 ZeusiRCd :" + Utils::make_string(user->mLang, "You havent used the NICK command yet, you have limited access."));
			return;
		}
		else if (results.size() < 2) return;
		Channel* chan = Mainframe::instance()->getChannelByName(results[1]);
		if (chan) {
			if (chan->hasUser(user) == false) {
				user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "You are not into the channel."));
				return;
			}
			chan->increaseflood();
			user->cmdPart(chan);
			Server::Send("SPART " + user->mNickName + " " + chan->name());
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Part);
}
