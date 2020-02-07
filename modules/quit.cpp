#include "ZeusBaseClass.h"
#include "module.h"

class CMD_Quit : public Module
{
	public:
	CMD_Quit() : Module("QUIT", 50, false) {};
	~CMD_Quit() {};
	virtual void command(LocalUser *user, std::string message) override {
		if (user->quit == false) {
			user->quit = true;
			user->Exit(true);
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Quit);
}
