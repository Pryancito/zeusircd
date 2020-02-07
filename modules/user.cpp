#include "ZeusBaseClass.h"
#include "module.h"
#include "Server.h"

class CMD_User : public Module
{
	public:
	CMD_User() : Module("USER", 50, false) {};
	~CMD_User() {};
	virtual void command(LocalUser *user, std::string message) override {
		std::string ident;
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		if (results.size() < 5) {
			user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "More data is needed."));
			return;
		} if (results[1].length() > 10)
			ident = results[1].substr(0, 9);
		else
			ident = results[1];
		user->mIdent = ident;
		Server::Send("SUSER " + user->mNickName + " " + ident);
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_User);
}
