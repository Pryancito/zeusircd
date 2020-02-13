#include "ZeusBaseClass.h"
#include "module.h"
#include "mainframe.h"

extern OperSet miRCOps;

class CMD_Opers : public Module
{
	public:
	CMD_Opers() : Module("OPERS", 50, false) {};
	~CMD_Opers() {};
	virtual void command(LocalUser *user, std::string message) override {
		for (auto oper : miRCOps) {
			LocalUser *usr = Mainframe::instance()->getLocalUserByName(oper);
			if (usr) {
				std::string online = " ( \0033ONLINE\003 )";
				if (usr->bAway)
					online = " ( \0034AWAY\003 )";
				user->SendAsServer("002 " + user->mNickName + " :" + usr->mNickName + online);
			}
			RemoteUser *ruser = Mainframe::instance()->getRemoteUserByName(oper);
			if (ruser) {
				std::string online = " ( \0033ONLINE\003 )";
				if (ruser->bAway)
					online = " ( \0034AWAY\003 )";
				user->SendAsServer("002 " + user->mNickName + " :" + ruser->mNickName + online);
			}
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Opers);
}
