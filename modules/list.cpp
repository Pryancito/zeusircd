#include "ZeusBaseClass.h"
#include "module.h"
#include "Utils.h"
#include "mainframe.h"

class CMD_List : public Module
{
	public:
	CMD_List() : Module("LIST", 50, false) {};
	~CMD_List() {};
	virtual void command(LocalUser *user, std::string message) override {
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		if (!user->bSentNick) {
			user->SendAsServer("461 ZeusiRCd :" + Utils::make_string(user->mLang, "You havent used the NICK command yet, you have limited access."));
			return;
		}
		std::string comodin = "*";
		if (results.size() == 2)
			comodin = results[1];
		user->SendAsServer("321 " + user->mNickName + " " + Utils::make_string(user->mLang, "Channel :Users Name"));
		auto channels = Mainframe::instance()->channels();
		auto it = channels.begin();
		for (; it != channels.end(); ++it) {
			std::string mtch = it->second->name();
			std::transform(comodin.begin(), comodin.end(), comodin.begin(), ::tolower);
			std::transform(mtch.begin(), mtch.end(), mtch.begin(), ::tolower);
			if (Utils::Match(comodin.c_str(), mtch.c_str()) == 1)
				user->SendAsServer("322 " + user->mNickName + " " + it->second->name() + " " + std::to_string(it->second->userCount()) + " :" + it->second->topic());
		}
		user->SendAsServer("323 " + user->mNickName + " :" + Utils::make_string(user->mLang, "End of /LIST"));
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_List);
}
