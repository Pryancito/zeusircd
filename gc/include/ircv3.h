#ifndef IRCV3_H
#define IRCV3_H

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
		bool use_uh_in_names;
		bool use_extended_join;
		bool use_image_base64;
		
	public:
		explicit Ircv3 (User *u) : mUser(u), batch(false), negotiating(false), usev3(false), use_batch(false), use_away_notify(false), use_uh_in_names(false), use_extended_join(false), use_image_base64(false) {
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

#endif