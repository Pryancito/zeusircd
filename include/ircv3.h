#pragma once

#define GC_THREADS
#define GC_ALWAYS_MULTITHREADED
#include <gc_cpp.h>
#include <gc.h>

#include <string>

#include "user.h"
#include "config.h"

class Ircv3 : public gc_cleanup
{
	private:
		User *mUser;
		std::string version = "302";
		bool negotiating;
		bool usev3;
		bool use_away_notify;
		bool use_uh_in_names;
		bool use_extended_join;
		
	public:
		explicit Ircv3 (User *u) : mUser(u), negotiating(false), usev3(false), use_away_notify(false), use_uh_in_names(false), use_extended_join(false) {
			if (config->Getvalue("ircv3") == "true" || config->Getvalue("ircv3") == "1")
				usev3 = true;
			else
				usev3 = false;
		};
		void sendCAP(const std::string &cmd);
		void recvEND();
		void Request(std::string request);
		bool HasCapab(const std::string &capab);
		std::string sts();
};

