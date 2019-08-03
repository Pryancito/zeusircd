#include "ZeusBaseClass.h"
#include <string>

void LocalUser::Parse(std::string message)
{
	this->Send( message );
}
