#include "ZeusiRCd.h"
#include "module.h"

extern time_t encendido;

class CMD_Uptime : public Module
{
	public:
	CMD_Uptime() : Module("UPTIME", 50, false) {};
	~CMD_Uptime() {};
	virtual void command(User *user, std::string message) override {
		user->SendAsServer("002 " + user->mNickName + " :" + Utils::make_string(user->mLang, "This server started as long as: %s", Utils::Time(encendido).c_str()));
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Uptime);
}
