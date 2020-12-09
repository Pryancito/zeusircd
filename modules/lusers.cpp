#include "ZeusiRCd.h"
#include "module.h"
#include "Utils.h"
#include "services.h"

class CMD_Lusers : public Module
{
	public:
	CMD_Lusers() : Module("LUSERS", 50, false) {};
	~CMD_Lusers() {};
	virtual void command(User *user, std::string message) override {
		user->SendAsServer("002 " + user->mNickName + " :" + Utils::make_string(user->mLang, "There are \002%s\002 users and \002%s\002 channels.", std::to_string(Users.size()).c_str(), std::to_string(Channels.size()).c_str()));
		user->SendAsServer("002 " + user->mNickName + " :" + Utils::make_string(user->mLang, "There are \002%s\002 registered nicks and \002%s\002 registered channels.", std::to_string(NickServ::GetNicks()).c_str(), std::to_string(ChanServ::GetChans()).c_str()));
		user->SendAsServer("002 " + user->mNickName + " :" + Utils::make_string(user->mLang, "There are \002%s\002 connected iRCops.", std::to_string(Oper::Count()).c_str()));
		user->SendAsServer("002 " + user->mNickName + " :" + Utils::make_string(user->mLang, "There are \002%s\002 connected servers.", std::to_string(Server::count()).c_str()));
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Lusers);
}
