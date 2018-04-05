#pragma once

#include <string>

#include "user.h"
#include "config.h"

class Ircv3
{
	private:
		User *mUser;
		std::string version = "302";
		bool batch;
		bool negotiating;
		
	public:
		Ircv3 (User *u);
		void sendCAP(std::string cmd);
		void recvEND();
		std::string sts();
};
