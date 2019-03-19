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
*/
#include "config.h"
#include "mainframe.h"

#define GC_THREADS
#define GC_ALWAYS_MULTITHREADED
#include <gc.h>
#include <gc_cpp.h>

using namespace std;

Config *config = new Config();

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
    boost::split(x,linea,boost::is_any_of("=\r\n\t"));
	Configura(x[0], x[1]);
}

void Config::Configura (string dato, const string &valor) {
	conf[dato] = valor;
}

string Config::Getvalue (string dato) {
	return conf[dato];
}

void Config::MainSocket(std::string ip, int port, bool ssl, bool ipv6) {
    GC_stack_base sb;
    GC_get_stack_base(&sb);
    GC_register_my_thread(&sb);
	try {
		Mainframe* frame = Mainframe::instance();
		frame->start(ip, port, ssl, ipv6);
	} catch (std::exception& e) {
		std::cout << "ERROR on socket: " << e.what() << std::endl;
	}
	GC_unregister_my_thread();
}

void Config::ServerSocket(std::string ip, int port, bool ssl, bool ipv6) {
    GC_stack_base sb;
    GC_get_stack_base(&sb);
    GC_register_my_thread(&sb);
	try {
		Mainframe* frame = Mainframe::instance();
		frame->server(ip, port, ssl, ipv6);
	} catch (std::exception& e) {
		std::cout << "ERROR on socket: " << e.what() << std::endl;
	}
	GC_unregister_my_thread();
}

void Config::WebSocket(std::string ip, int port, bool ssl, bool ipv6) {
    GC_stack_base sb;
    GC_get_stack_base(&sb);
    GC_register_my_thread(&sb);
	try {
		Mainframe* frame = Mainframe::instance();
		frame->ws(ip, port, ssl, ipv6);
	} catch (std::exception& e) {
		std::cout << "ERROR on socket: " << e.what() << std::endl;
	}
	GC_unregister_my_thread();
}
