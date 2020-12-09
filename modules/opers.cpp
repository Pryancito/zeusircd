#include "ZeusiRCd.h"
#include "module.h"

extern OperSet miRCOps;

class CMD_Opers : public Module
{
	public:
	CMD_Opers() : Module("OPERS", 50, false) {};
	~CMD_Opers() {};
	virtual void command(User *user, std::string message) override {
		for (auto oper : miRCOps) {
			User *usr = User::GetUser(oper);
			if (usr) {
				std::string online = " ( \0033ONLINE\003 )";
				if (usr->bAway)
					online = " ( \0034AWAY\003 )";
				user->SendAsServer("002 " + user->mNickName + " :" + usr->mNickName + online);
			}
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Opers);
}
