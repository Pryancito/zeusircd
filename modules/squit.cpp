#include "ZeusBaseClass.h"
#include "module.h"
#include "Utils.h"
#include "Server.h"
#include "Config.h"

class CMD_Squit : public Module
{
	public:
	CMD_Squit() : Module("SQUIT", 50, false) {};
	~CMD_Squit() {};
	virtual void command(LocalUser *user, std::string message) override {
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		if (results.size() < 2) {
			user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		} else if (user->getMode('o') == false) {
			user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "You do not have iRCop privileges."));
			return;
		} else if (Server::Exists(results[1]) == false) {
			user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The server is not connected."));
			return;
		} else if (strcasecmp(results[1].c_str(), config["serverName"].as<std::string>().c_str()) == 0) {
			user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "You can not make an SQUIT to your own server."));
			return;
		} else {
			Server::SQUIT(results[1], true, true);
			user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The server has been disconnected."));
			return;
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Squit);
}
