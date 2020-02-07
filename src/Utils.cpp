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
 * This product includes GeoLite2 data created by MaxMind, available from
 * https://www.maxmind.com
*/

#include "Utils.h"
#include "Config.h"
#include "GeoLite2PP.h"
#include "i18n.h"
#include "services.h"
#include "mainframe.h"
#include "Server.h"

#include <stdarg.h>
#include <iostream>
#include <string>
#include <fstream>

using namespace std;

std::mutex log_mtx;
std::map<std::string, std::pair<std::string, std::string>> cache;

bool Utils::isnum(const std::string &cadena)
{
  for(unsigned int i = 0; i < cadena.length(); i++)
  {
    if( !std::isdigit(cadena[i]) )
      return false;
  }

  return true;
}

bool Utils::Match(const char *first, const char *second)
{
    // If we reach at the end of both strings, we are done
    if (*first == '\0' && *second == '\0')
        return true;

    // Make sure that the characters after '*' are present
    // in second string. This function assumes that the first
    // string will not contain two consecutive '*'
    if (*first == '*' && *(first+1) != '\0' && *second == '\0')
        return false;

    // If the first string contains '?', or current characters
    // of both strings match
    if (*first == '?' || *first == *second)
        return Match(first+1, second+1);

    // If there is *, then there are two possibilities
    // a) We consider current character of second string
    // b) We ignore current character of second string.
    if (*first == '*')
        return Match(first+1, second) || Match(first, second+1);
    return false;
}

std::string Utils::Time(time_t tiempo) {
	int dias, horas, minutos, segundos = 0;
	std::string total;
	time_t now = time(0);
	tiempo = now - tiempo;
	if (tiempo <= 0)
		return "0s";
	dias = tiempo / 86400;
	tiempo = tiempo - ( dias * 86400 );
	horas = tiempo / 3600;
	tiempo = tiempo - ( horas * 3600 );
	minutos = tiempo / 60;
	tiempo = tiempo - ( minutos * 60 );
	segundos = tiempo;

	if (dias) {
		total.append(std::to_string(dias));
		total.append("d ");
	} if (horas) {
		total.append(std::to_string(horas));
		total.append("h ");
	} if (minutos) {
		total.append(std::to_string(minutos));
		total.append("m ");
	} if (segundos) {
		total.append(std::to_string(segundos));
		total.append("s");
	}
	return total;
}

time_t Utils::UnixTime(std::string time)
{
	time_t tiempo = 0;
	std::string cadena = "0";
	for (unsigned int i = 0; i < time.size(); i++)
	{
		if (time[i] == 's') {
			tiempo += (time_t ) stoi(cadena);
			cadena.clear();
		} else if (time[i] == 'm') {
			tiempo += (time_t ) stoi(cadena) * 60;
			cadena.clear();
		} else if (time[i] == 'h') {
			tiempo += (time_t ) stoi(cadena) * 3600;
			cadena.clear();
		} else if (time[i] == 'd') {
			tiempo += (time_t ) stoi(cadena) * 86400;
			cadena.clear();
		}
		else if (isdigit(time[i]) == true)
		{
			cadena += time[i];
		}
	}
	return tiempo;
}

std::string Utils::PartialTime(time_t tiempo) {
	int dias, horas, minutos, segundos = 0;
	std::string total;
	if (tiempo <= 0)
		return "0s";
	dias = tiempo / 86400;
	tiempo = tiempo - ( dias * 86400 );
	horas = tiempo / 3600;
	tiempo = tiempo - ( horas * 3600 );
	minutos = tiempo / 60;
	tiempo = tiempo - ( minutos * 60 );
	segundos = tiempo;

	if (dias) {
		total.append(std::to_string(dias));
		total.append("d ");
	} if (horas) {
		total.append(std::to_string(horas));
		total.append("h ");
	} if (minutos) {
		total.append(std::to_string(minutos));
		total.append("m ");
	} if (segundos) {
		total.append(std::to_string(segundos));
		total.append("s");
	}
	return total;
}

std::string Utils::make_string(const std::string &lang, const std::string fmt, ...) {
	if (cache.find(fmt) != cache.end()) {
		if (cache[fmt].first == lang) {
			std::string msg = cache[fmt].second;
			char buffer[512];
			va_list args;
			va_start (args, fmt);
			std::vsnprintf (buffer, 512, msg.c_str(), args);
			va_end (args);
			return buffer;
		}
	}
	LauGettext getxt;
	getxt.setCatalogueName("zeus");
	getxt.setCatalogueLocation("lang");

    if (lang != "") {
		getxt.setLocale(lang);
	} else if (config["language"]) {
		getxt.setLocale(config["language"].as<std::string>());
	} else
		getxt.setLocale("en");
	GettextMessage* message = getxt.getTranslation(fmt.c_str(), strlen(fmt.c_str()));

	std::string msg;
	if (!message) {
	   msg = fmt;
	} else {
	   msg = message->string;
	}

	cache.insert( std::make_pair( fmt, std::make_pair(lang,msg) ) );

	char buffer[512];
	va_list args;
	va_start (args, fmt);
	std::vsnprintf (buffer, 512, msg.c_str(), args);
	va_end (args);
	return buffer;
}

char toFlagByte(char c) { return 0x65 + c; }

std::string Utils::GetEmoji(const std::string &ip) {
	std::string country = GetGeoIP(ip);
	
	if (country == "ERROR") return "ERROR";
	
	char flag[] = {
			(char)0xF0, (char)0x9F, (char)0x87, toFlagByte(country[0]),
			(char)0xF0, (char)0x9F, (char)0x87, toFlagByte(country[1]),
		0};
	return "[ " + country + " ]" + " - " + flag;
}
	
std::string Utils::GetGeoIP(const std::string &ip) {
	std::string country = "";
	GeoLite2PP::MStr m;
	try {
		GeoLite2PP::DB db( "GeoLite2-Country.mmdb" );
		m = db.get_all_fields( ip );
	} catch (...) {
		return "ERROR";
	}
	for ( const auto iter : m )
	{
		if (iter.first == "country_iso_code")
			country = iter.second;
	}
	
	if (country == "") return "ERROR";
	return country;
}

void Utils::split(const std::string& str, std::vector<std::string>& cont, const std::string& delims)
{
    std::size_t start = str.find_first_not_of(delims), end = 0;

    while((end = str.find_first_of(delims, start)) != std::string::npos)
    {
        cont.push_back(str.substr(start, end - start));
        start = str.find_first_not_of(delims, end);
    }
    if(start != std::string::npos)
        cont.push_back(str.substr(start));
}

void Utils::log(const std::string &message) {
	log_mtx.lock();
	Channel *chan = Mainframe::instance()->getChannelByName("#debug");
	if (config["hub"].as<std::string>() == config["serverName"].as<std::string>()) {
		time_t now = time(0);
		struct tm tm;
		localtime_r(&now, &tm);
		char date[32];
		strftime(date, sizeof(date), "%r %d-%m-%Y", &tm);
		std::ofstream fileLog("ircd.log", std::ios::out | std::ios::app);
		if (fileLog.fail()) {
			if (chan)
				chan->broadcast(":" + config["operserv"].as<std::string>() + " PRIVMSG #debug :Error opening log file.");
			log_mtx.unlock();
			return;
		}
		fileLog << date << " -> " << message << std::endl;
		fileLog.close();
	}
	if (chan) {
		chan->broadcast(":" + config["operserv"].as<std::string>() + " PRIVMSG #debug :" + message);
		Server::Send("PRIVMSG " + config["operserv"].as<std::string>() + " #debug " + message);
	}
	log_mtx.unlock();
}

bool Utils::checkstring (const std::string &str) {
	if (str.length() == 0)
		return false;
	for (unsigned int i = 0; i < str.length(); i++)
		if (!std::isalnum(str[i]))
			return false;
	return true;
}

bool Utils::checknick (const std::string &nick) {
	if (nick.length() == 0)
		return false;
	if (!std::isalpha(nick[0]))
		return false;
	if (nick.find("'") != std::string::npos || nick.find("\"") != std::string::npos || nick.find(";") != std::string::npos
		|| nick.find("@") != std::string::npos || nick.find("*") != std::string::npos || nick.find("/") != std::string::npos
		|| nick.find(",") != std::string::npos)
		return false;
	return true;
}

bool Utils::checkchan (const std::string &chan) {
	if (chan.length() == 0)
		return false;
	if (chan[0] != '#')
		return false;
	if (chan.find("'") != std::string::npos || chan.find("\"") != std::string::npos || chan.find(";") != std::string::npos
		|| chan.find("@") != std::string::npos || chan.find("*") != std::string::npos || chan.find("/") != std::string::npos
		|| chan.find(",") != std::string::npos)
		return false;
	return true;
}
