/* 
 * This file is part of the ZeusiRCd distribution (https://github.com/Pryancito/zeusircd).
 * Copyright (c) 2019 Rodrigo Santidrian AKA Pryan.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * This file include code from some part of github i can't remember.
*/

#pragma once

#include <dlfcn.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "ZeusiRCd.h"

typedef void* dynamic_lib_handle;

struct dynamic_lib {
	std::string			path;
	dynamic_lib_handle  handle;

	dynamic_lib(std::string p) : path(p), handle(nullptr) {}

	~dynamic_lib() {
		if (handle != nullptr)
			dlclose(handle);
	}
};

class Widget {

  public:
	Widget(std::string comando, unsigned int pr, bool end) : cmd(comando), priority(pr), must_end(end) {};
	virtual ~Widget() {};
	std::string			cmd;
	int				priority = 0;
	bool				must_end = false;
	virtual void command(User *user, std::string message = "") = 0;
};

extern std::vector<Widget*> commands;
extern std::vector<dynamic_lib> modules;

class Module : public Widget
{
  public:
	Module(std::string cmd, unsigned int pr, bool end) : Widget(cmd, pr, end) {};
	virtual ~Module() {};
	static int LoadAll();
	static int UnloadAll();
	static void LoadModule(std::string module);
	static void UnloadModule(std::string module);
	static dynamic_lib_handle load_lib(const std::string& path);
	static Widget *instantiate(const dynamic_lib_handle handle);
};
