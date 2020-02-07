#pragma once

#include <dlfcn.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "ZeusBaseClass.h"

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
	int					priority = 0;
	bool				must_end = false;
	virtual void command(LocalUser *user, std::string message = "") = 0;
};

extern std::vector<Widget*> commands;
extern std::vector<dynamic_lib> modules;

class Module : public Widget
{
	public:
		Module(std::string cmd, unsigned int pr, bool end) : Widget(cmd, pr, end) {};
		virtual ~Module() {};
		static void LoadAll();
		static void UnloadAll();
		static void LoadModule(std::string module);
		static void UnloadModule(std::string module);
		static dynamic_lib_handle load_lib(const std::string& path);
		static Widget *instantiate(const dynamic_lib_handle handle);
};
