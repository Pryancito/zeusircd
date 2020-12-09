#include "ZeusiRCd.h"
#include "module.h"

class CMD_Pong : public Module
{
	public:
	CMD_Pong() : Module("PONG", 50, false) {};
	~CMD_Pong() {};
	virtual void command(User *user, std::string message) override {
		user->bPing = time(0);
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Pong);
}
