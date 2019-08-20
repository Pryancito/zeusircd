#include <algorithm>
#include <iterator>

#include "Config.h"

using namespace std;
Config *config = new Config();

template <class Container>
void Config::split(const std::string& str, Container& cont, const std::string& delims)
{
    std::size_t current, previous = 0;
    current = str.find_first_of(delims);
    while (current != std::string::npos) {
        cont.push_back(str.substr(previous, current - previous));
        previous = current + 1;
        current = str.find_first_of(delims, previous);
    }
    cont.push_back(str.substr(previous, current - previous));
}

void Config::Cargar () {
	string linea;
	ifstream fichero(file);
	while (!fichero.eof()) {
		getline(fichero, linea);
		if (linea[0] != '#' && linea.length() > 0)
			Procesa(linea);
	}
	fichero.close();
}

void Config::Procesa (string linea) {
    vector<string> x;
    split(linea, x, "=\r\n\t");
    if (x[0] == "database")
		DBConfig(x[0], x[1]);
	else
		Configura(x[0], x[1]);
}

void Config::DBConfig(std::string dato, std::string uri) {
	vector<string> x;
	split(uri, x, ":/@#");
    if (x[0] == "mysql") {
		Configura("dbtype", x[0]);
		Configura("dbuser", x[3]);
		Configura("dbpass", x[4]);
		Configura("dbhost", x[5]);
		Configura("dbport", x[6]);
		Configura("dbname", x[7]);
		if (x.size() > 8) {
			if (x[8] == "cluster")
				Configura("cluster", "true");
			else
				Configura("cluster", "false");
		} else
				Configura("cluster", "false");
	} else {
		Configura("dbtype", "sqlite3");
		Configura("cluster", "false");
	}
}

void Config::Configura (string dato, const string &valor) {
	conf[dato] = valor;
}

string Config::Getvalue (string dato) {
	return conf[dato];
}