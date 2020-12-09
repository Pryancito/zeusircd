#include "ZeusiRCd.h"
#include "module.h"
#include "Utils.h"
#include "Server.h"
#include "Config.h"

class CMD_Connect : public Module
{
	public:
	CMD_Connect() : Module("CONNECT", 50, false) {};
	~CMD_Connect() {};
	virtual void command(User *user, std::string message) override {
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		if (results.size() < 2) {
			user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		} else if (user->getMode('o') == false) {
			user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "You do not have iRCop privileges."));
			return;
		} else if (Server::IsAServer(results[1]) == false) {
			user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The server is not listed in config."));
			return;
		} else if (Server::IsConected(results[1]) == true) {
			user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The server is already connected."));
			return;
		} else {
			user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "Connecting ..."));
			for (unsigned int i = 0; i < config["links"].size(); i++) {
				if (config["links"][i]["ip"].as<std::string>() == results[1]) {
					Server *srv = new Server(config["links"][i]["ip"].as<std::string>(), config["links"][i]["port"].as<std::string>());
					Servers.insert(srv);
					srv->send("BURST");
					Server::sendBurst(srv);
				}
			}
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Connect);
}
