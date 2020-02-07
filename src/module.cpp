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
			exit(0);
		}
		modules.push_back(dynamic_lib(module));
	} catch (std::exception& e) {
		std::cout << e.what() << std::endl;
		exit(0);
	}
}

void Module::UnloadModule(std::string module)
{
	for (auto& m : modules) {
		if (m.path == module) {
			try {
				std::cout << "Unloading: " << module << std::endl;
				dlclose(m.handle);
			} catch (std::exception& e) {
				std::cerr << e.what() << std::endl;
				exit(0);
			}
		} else
			std::cout << "Module not exists: " << module << std::endl;
	}
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
		void *module = dlopen(m.path.c_str(), RTLD_LAZY);
		if (!module) {
			std::cout << "Cannot load library: " << dlerror() << '\n';
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
	for (auto& m : modules) {
		UnloadModule(m.path);
		unloaded++;
	}
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
