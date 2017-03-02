#include "include.h"
#include <vector>
#include <sstream>

using namespace std;

Config *config = new Config();

vector<string> split(const string& str){
	string palabra1, palabra2;
	bool igual = false;
	for (unsigned int i = 0; i < str.length(); i++) {
		if (str[i] == '=')
			igual = true;
		else if (igual == false)
			palabra1.push_back(str[i]);
		else if (igual == true && str[i] != '\r' && str[i] != '\n')
			palabra2.push_back(str[i]);
	}

	return { palabra1, palabra2 };
}

void Config::Cargar () {
	string linea;
	ifstream fichero("server.conf");
	while (!fichero.eof()) {
		getline(fichero, linea);
		Procesa(linea);
	}
	fichero.close();
	return;
}

void Config::Procesa (string linea) {
    vector<string> x = split(linea);
	Configura(x[0], x[1]);
    return;
}

void Config::Configura (string dato, string valor) {
	conf[dato] = valor;
	return;
}

string Config::Getvalue (string dato) {
	return conf[dato];
}
