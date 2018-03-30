#pragma once

#include <set>
#include <string>
#include "user.h"
#include "session.h"

typedef std::set<User*> OperSet; 

class Oper
{
	public:
		bool Login (User *u, std::string nickname, std::string pass);
		void 		GlobOPs (std::string mensaje);
		std::string MkPassWD (std::string pass);
		bool	IsOper(User *u);
		static int 		Count ();
};
