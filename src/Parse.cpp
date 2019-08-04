#include "ZeusBaseClass.h"
#include <string>
#include <iterator>

std::string& ltrim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
{
    str.erase(0, str.find_first_not_of(chars));
    return str;
}
 
std::string& rtrim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
{
    str.erase(str.find_last_not_of(chars) + 1);
    return str;
}
 
std::string& trim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
{
    return ltrim(rtrim(str, chars), chars);
}

void LocalUser::Parse(std::string message)
{
	trim(message);
	std::istringstream iss(message);
	std::vector<std::string> results(std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>());
	
	if (results.size() == 0)
		return;
	
	std::string cmd = results[0];

	if (cmd == "QUIT") {
		//if (getMode('o') == true)
		//	miRCOps.erase(this);
		//Servidor::sendall("QUIT " + mNickName);
		quit = true;
	}
}
