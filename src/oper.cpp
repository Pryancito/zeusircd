#include "oper.h"
#include "sha256.h"
#include "config.h"
#include "utils.h"

#include <string>

OperSet miRCOps;

bool Oper::Login (User* user, std::string nickname, std::string pass) {
	for (unsigned int i = 0; config->Getvalue("oper["+std::to_string(i)+"]nick").length() > 0; i++)
		if (config->Getvalue("oper["+std::to_string(i)+"]nick") == nickname)
			if (config->Getvalue("oper["+std::to_string(i)+"]pass") == sha256(pass)) {
				miRCOps.insert(user);
				user->session()->sendAsServer("MODE " + user->nick() + " +o" + config->EOFMessage);
				user->setMode('o', true);
				Servidor::sendall("UMODE " + user->nick() + " +o");
				return true;
			}
	GlobOPs(Utils::make_string("", "Failure /oper auth from nick: %s", user->nick().c_str()));
	return false;
}

void Oper::GlobOPs(std::string message) {
	OperSet::iterator it = miRCOps.begin();
    for(; it != miRCOps.end(); ++it) {
		if ((*it)->server() == config->Getvalue("serverName"))
			(*it)->session()->sendAsServer("NOTICE " + (*it)->nick() + " :" + message + config->EOFMessage);
		else
			Servidor::sendall("NOTICE " + config->Getvalue("serverName") + " " + (*it)->nick() + " " + message);
    }
}

std::string Oper::MkPassWD (std::string pass) {
	return sha256(pass);
}

bool Oper::IsOper(User* user) {
	return user->getMode('o');
}

int Oper::Count () {
	return miRCOps.size();
}
