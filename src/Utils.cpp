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

#include <stdarg.h>
#include <iostream>
#include <string>

using namespace std;

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
	boost::split(cont, str, boost::is_any_of(delims), boost::token_compress_on);
}
