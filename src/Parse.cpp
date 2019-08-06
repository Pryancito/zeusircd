#include "ZeusBaseClass.h"
#include "Config.h"
#include "Timer.h"
#include "Utils.h"

#include <string>
#include <iterator>
#include <algorithm>

TimerWheel timers;

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
	std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
	
	if (cmd == "QUIT") {
		quit = true;
		Close();
	} else if (cmd == "PING") {
		bPing = time(0);
		Send(":" + config->Getvalue("serverName") + " PONG " + config->Getvalue("serverName") + (results.size() > 2 ? results[1] : ""));
	} else if (cmd == "PONG") {
		bPing = time(0);
	}
}

void LocalUser::CheckNick() {
	if (!bSentNick) {
		quit = true;
		Close();
	}
};

void LocalUser::CheckPing() {
	if (bPing + 200 < time(0)) {
		quit = true;
		Close();
	} else {
		Send("PING :" + config->Getvalue("serverName"));
		StartTimers(&timers);
	}
};

void LocalUser::StartTimers(TimerWheel* timers)
{
	if (!bSentNick)
		timers->schedule(&tnick, 10);
	timers->schedule(&tping, 60);
}
