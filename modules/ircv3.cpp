#include "ZeusBaseClass.h"
#include "module.h"

class IRCv3 : public Module
{
	public:
	IRCv3() : Module("CAP") {};
	~IRCv3() {};
	virtual void command(LocalUser *user, std::string message) override {
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		if (results.size() < 2) return;
		else if (results[1] == "LS" || results[1] == "LIST")
			sendCAP(user, results[1]);
		else if (results[1] == "REQ")
			Request(user, message);
		else if (results[1] == "END") {
			user->negotiating = false;
		}
	}
	void sendCAP(LocalUser *user, const std::string &cmd) {
		user->negotiating = true;
		user->SendAsServer("CAP * " + cmd + " :away-notify userhost-in-names" + sts(user));
	}

	void Request(LocalUser *user, std::string request) {
		std::string capabs = ":";
		std::string req = request.substr(9);
		std::vector<std::string>  x;
		Utils::split(req, x, " ");
		for (unsigned int i = 0; i < x.size(); i++) {
			if (x[i] == "away-notify") {
				capabs.append(x[i] + " ");
				user->away_notify = true;
			} else if (x[i] == "userhost-in-names") {
				capabs.append(x[i] + " ");
				user->userhost_in_names = true;
			} else if (x[i] == "extended-join") {
				capabs.append(x[i] + " ");
				user->extended_join = true;
			}
		}
		user->SendAsServer("CAP * ACK " + capabs);
	}

	std::string sts(LocalUser *user) {
		int puerto = 0;
		if (user->mHost.find(":") != std::string::npos) {
			for (unsigned int i = 0; i < config["listen6"].size(); i++) {
				if (config["listen6"][i]["class"].as<std::string>() == "client" && config["listen6"][i]["ssl"].as<bool>() == true)
					puerto = config["listen6"][i]["port"].as<int>();
			}
		} else {
			for (unsigned int i = 0; i < config["listen"].size(); i++) {
				if (config["listen"][i]["class"].as<std::string>() == "client" && config["listen"][i]["ssl"].as<bool>() == true)
					puerto = config["listen"][i]["port"].as<int>();
			}
		}
		if (puerto == 0)
			return "";
		else
			return " sts=port=" + std::to_string(puerto) + ",duration=10";
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new IRCv3);
}
