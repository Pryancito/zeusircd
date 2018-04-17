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
		bool usev3;
		bool use_batch;
		bool use_away_notify;
		
	public:
		Ircv3 (User *u);
		void sendCAP(std::string cmd);
		void recvEND();
		void Request(std::string request);
		std::string sts();
};
