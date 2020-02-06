#include "Config.h"
#include "module.h"

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
		std::cerr << e.what() << std::endl;
		exit(0);
	}

	for (auto& m : modules) {
		m.handle = load_lib(m.path);
		commands.push_back( instantiate(m.handle) );
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

void Module::LoadAll()
{
	for (unsigned int i = 0; i < config["modules"].size(); i++)
	{
		LoadModule(config["modules"][i]["path"].as<std::string>());
	}
}	

void Module::UnloadAll()
{
	for (auto& m : modules) {
		UnloadModule(m.path);
	}
}

dynamic_lib_handle Module::load_lib(const std::string& path) {
	return dlopen(path.data() , RTLD_NOW);
}

Widget *Module::instantiate(const dynamic_lib_handle handle) {

	if (handle == nullptr) return nullptr;

	void *maker = dlsym(handle , "factory");

	if (maker == nullptr) return nullptr;

	typedef Widget* (*fptr)();
	fptr func = reinterpret_cast<fptr>(reinterpret_cast<void*>(maker));
	return func();
}
