#include "ZeusBaseClass.h"
#include "module.h"

class CMD_Pass : public Module
{
	public:
	CMD_Pass() : Module("PASS", 50, false) {};
	~CMD_Pass() {};
	virtual void command(LocalUser *user, std::string message) override {
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		if (results.size() < 2) {
			user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		}
		user->PassWord = results[1];
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Pass);
}
