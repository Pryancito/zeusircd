#include "ZeusiRCd.h"
#include "module.h"
#include "Utils.h"
#include "Config.h"

class CMD_Webirc : public Module
{
	public:
	CMD_Webirc() : Module("WEBIRC", 50, false) {};
	~CMD_Webirc() {};
	virtual void command(User *user, std::string message) override {
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		if (results.size() < 5) {
			user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		} else if (results[1] == config["cgiirc"].as<std::string>()) {
			user->mHost = results[4];
			return;
		} else {
			user->Exit(true);
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Webirc);
}
