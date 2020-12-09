#include "ZeusiRCd.h"
#include "module.h"

class CMD_Topic : public Module
{
	public:
	CMD_Topic() : Module("TOPIC", 50, false) {};
	~CMD_Topic() {};
	virtual void command(User *user, std::string message) override {
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		if (!user->bSentNick) {
			user->SendAsServer("461 ZeusiRCd :" + Utils::make_string(user->mLang, "You havent used the NICK command yet, you have limited access."));
			return;
		}
		else if (results.size() == 2) {
			Channel* chan = Channel::GetChannel(results[1]);
			if (chan) {
				if (chan->mTopic.empty()) {
					user->SendAsServer("331 " + chan->name + " :" + Utils::make_string(user->mLang, "No topic is set !"));
				}
				else {
					user->SendAsServer("332 " + user->mNickName + " " + chan->name + " :" + chan->mTopic);
				}
			}
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Topic);
}
