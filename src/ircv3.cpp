#include "ircv3.h"
#include "session.h"

void Ircv3::sendCAP(const std::string &cmd) {
	negotiating = true;
	if (usev3 == true)
		mUser->session()->sendAsServer("CAP * " + cmd + " :batch away-notify userhost-in-names extended-join image-base64" + sts() + config->EOFMessage);
	else if (mUser->session()->websocket == true)
		mUser->session()->sendAsServer("CAP * " + cmd + " :batch" + config->EOFMessage);
}

void Ircv3::recvEND() {
	negotiating = false;
	std::string capabs = ":";
	if (use_batch == true)
		capabs.append("batch ");
	if (use_away_notify == true)
		capabs.append("away-notify ");
	if (use_uh_in_names == true)
		capabs.append("userhost-in-names ");
	if (use_extended_join == true)
		capabs.append("extended-join ");
	if (use_image_base64 == true)
		capabs.append("image-base64 ");
	mUser->session()->sendAsServer("CAP " + mUser->nick() + " ACK " + capabs + config->EOFMessage);
}

void Ircv3::Request(std::string request) {
	std::string req = request.substr(9);
	StrVec  x;
	boost::split(x, req, boost::is_any_of(" \t"), boost::token_compress_on);
	for (unsigned int i = 0; i < x.size(); i++) {
		if (x[i] == "batch")
			use_batch = true;
		else if (x[i] == "away-notify")
			use_away_notify = true;
		else if (x[i] == "userhost-in-names")
			use_uh_in_names = true;
		else if (x[i] == "extended-join")
			use_extended_join = true;
		else if (x[i] == "image-base64")
			use_image_base64 = true;
	}
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

bool Ircv3::HasCapab(const std::string &capab) {
	if (capab == "batch")
		return use_batch;
	else if (capab == "away-notify")
		return use_away_notify;
	else if (capab == "userhost-in-names")
		return use_uh_in_names;
	else if (capab == "extended-join")
		return use_extended_join;
	else if (capab == "image-base64")
		return use_image_base64;
	else
		return false;
}
