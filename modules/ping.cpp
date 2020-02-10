#include "ZeusBaseClass.h"
#include "module.h"
#include "Utils.h"

class CMD_Ping : public Module
{
	public:
	CMD_Ping() : Module("PING", 50, false) {};
	~CMD_Ping() {};
	virtual void command(LocalUser *user, std::string message) override {
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		user->bPing = time(0);
		user->SendAsServer("PONG " + config["serverName"].as<std::string>() + " :" + (results.size() > 1 ? results[1] : ""));
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Ping);
}
