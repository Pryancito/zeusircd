#pragma once

#include <set>
#include <string>
#include "user.h"
#include "session.h"

typedef std::set<User*> OperSet; 

class Oper
{
	public:
		bool Login (User *u, const std::string &nickname, const std::string &pass);
		void 		GlobOPs (const std::string &mensaje);
		bool	IsOper(User *u);
		static int 		Count ();
};
