#pragma once

#include <string>
#include <map>
#include <vector>
#include <fstream>

class Config
{
        public:
                std::map <std::string, std::string> conf;
                std::string version = "Zeus-5.0-alpha";
                std::string file = "zeus.conf";

        void Cargar ();
        void Procesa (std::string linea);
        void Configura (std::string dato, const std::string &valor);
        void DBConfig(std::string dato, std::string uri);
        std::string Getvalue (std::string dato);
};

extern Config *config;
