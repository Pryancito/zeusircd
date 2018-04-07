#include "ircv3.h"
#include "session.h"

Ircv3::Ircv3(User *u) : mUser(u) {
	if (config->Getvalue("ircv3") == "true" || config->Getvalue("ircv3") == "1")
		usev3 = true;
	else
		usev3 = false;
}

void Ircv3::sendCAP(std::string cmd) {
	negotiating = true;
	if (usev3 == true)
		mUser->session()->sendAsServer("CAP * " + cmd + " :batch away-notify" + sts() + config->EOFMessage);
}

void Ircv3::recvEND() {
	negotiating = false;
}

std::string Ircv3::sts() {
	int puerto = 0;
	if (mUser->session()->ip().find(":") != std::string::npos) {
		for (unsigned int i = 0; config->Getvalue("listen6["+std::to_string(i)+"]ip").length() > 0; i++) {
			if (config->Getvalue("listen6["+std::to_string(i)+"]class") == "client" &&
				(config->Getvalue("listen6["+std::to_string(i)+"]ssl") == "1" || config->Getvalue("listen6["+std::to_string(i)+"]ssl") == "true"))
				puerto = (int) stoi(config->Getvalue("listen6["+std::to_string(i)+"]port"));
		}
	} else {
		for (unsigned int i = 0; config->Getvalue("listen["+std::to_string(i)+"]ip").length() > 0; i++) {
			if (config->Getvalue("listen["+std::to_string(i)+"]class") == "client" &&
				(config->Getvalue("listen["+std::to_string(i)+"]ssl") == "1" || config->Getvalue("listen["+std::to_string(i)+"]ssl") == "true"))
				puerto = (int) stoi(config->Getvalue("listen["+std::to_string(i)+"]port"));
		}
	}
	if (puerto == 0)
		return "";
	else
		return " sts=port=" + std::to_string(puerto) + ",duration=10";
}
