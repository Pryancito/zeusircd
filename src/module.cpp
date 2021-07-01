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

#include "Config.h"
#include "module.h"

#include <algorithm>

std::vector<dynamic_lib> modules;

std::vector<Widget*> commands;

void Module::LoadModule(std::string module)
{
	try {
		std::cout << "Loading: " << module << std::endl;
		if (access(module.c_str(), W_OK) != 0) {
			std::cout << "Module does not exists." << std::endl;
		}
		modules.push_back(dynamic_lib(module));
	} catch (std::exception& e) {
		std::cout << e.what() << std::endl;
	}
}

void Module::UnloadModule(std::string module)
{
	for (auto& m : modules) {
		if (m.path == module) {
			try {
				std::cout << "Unloading: " << module << std::endl;
				int result = dlclose(m.handle);
				if (result) {
					std::cout << "Cannot unload module: " << dlerror() << std::endl;
				}
			} catch (std::exception& e) {
				std::cerr << e.what() << std::endl;
			}
		}
	}
	modules.clear();
}

int Module::LoadAll()
{
	int loaded = 0;
	for (unsigned int i = 0; i < config["modules"].size(); i++)
	{
		LoadModule(config["modules"][i]["path"].as<std::string>());
		loaded++;
	}
	for (auto& m : modules) {
		void *module = dlopen(m.path.c_str(), RTLD_NOW);
		if (!module) {
			std::cout << "Cannot load module: " << dlerror() << std::endl;
			return -1;
		}
		m.handle = module;
		commands.push_back(instantiate(m.handle));
	}
	std::sort(commands.begin(), commands.end(), [](Widget* a, Widget* b) {
		return a->priority < b->priority;
	});
	return loaded;
}	

int Module::UnloadAll()
{
	int unloaded = 0;
	for (auto& c : commands) {
		delete c;
	}
	for (auto& m : modules) {
		try {
			std::cout << "Unloading: " << m.path << std::endl;
			int result = dlclose(m.handle);
			if (result != 0) {
				std::cout << "Cannot unload module: " << dlerror() << std::endl;
				return -1;
			} else
				unloaded++;
		} catch (std::exception& e) {
			std::cerr << e.what() << std::endl;
			return -1;
		}
	}
	commands.clear();
	modules.clear();
	return unloaded;
}

Widget *Module::instantiate(const dynamic_lib_handle handle) {
	if (handle == nullptr) return nullptr;
	void *maker = dlsym(handle , "factory");
	if (maker == nullptr) return nullptr;
	typedef Widget* (*fptr)();
	fptr func = reinterpret_cast<fptr>(reinterpret_cast<void*>(maker));
	return func();
}
