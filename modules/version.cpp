#include "ZeusiRCd.h"
#include "module.h"
#include "Config.h"
#include "db.h"

class CMD_Version : public Module
{
	public:
	CMD_Version() : Module("VERSION", 50, false) {};
	~CMD_Version() {};
	virtual void command(User *user, std::string message) override {
		user->SendAsServer("002 " + user->mNickName + " :" + Utils::make_string(user->mLang, "Version of ZeusiRCd: %s", config["version"].as<std::string>().c_str()));
		if (user->getMode('o') == true)
			user->SendAsServer("002 " + user->mNickName + " :" + Utils::make_string(user->mLang, "Version of DataBase: %s", DB::GetLastRecord().c_str()));
		}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Version);
}
