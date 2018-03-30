#include "oper.h"
#include "sha256.h"
#include "config.h"
#include <string>

OperSet miRCOps;

bool Oper::Login (User* user, std::string nickname, std::string pass) {
	for (unsigned int i = 0; config->Getvalue("oper["+std::to_string(i)+"]nick").length() > 0; i++)
		if (config->Getvalue("oper["+std::to_string(i)+"]nick") == nickname)
			if (config->Getvalue("oper["+std::to_string(i)+"]pass") == sha256(pass)) {
				miRCOps.insert(user);
				user->session()->sendAsServer("MODE " + user->nick() + " +o" + config->EOFMessage);
				user->setMode('o', true);
				return true;
			}
	GlobOPs("Intento fallido de autenticacion /oper del nick: " + user->nick());
	return false;
}

void Oper::GlobOPs(std::string message) {
	OperSet::iterator it = miRCOps.begin();
    for(; it != miRCOps.end(); ++it) {
        (*it)->session()->sendAsServer("NOTICE " + (*it)->nick() + " :" + message + config->EOFMessage);
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
