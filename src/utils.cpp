#define BOOST_LOCALE_HIDE_AUTO_PTR

#include "utils.h"
#include "services.h"
#include "config.h"
#include "json.h"

#include <stdarg.h>
#include <boost/locale.hpp>
#include <curl/curl.h>
#include <iostream>
#include <string>

using namespace boost::locale;
using namespace std;
using namespace json11;

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

std::string Utils::make_string(const std::string &nickname, const std::string& fmt, ...) {
    generator gen;
    locale loc;
    gen.add_messages_path("lang");
    gen.add_messages_domain("zeus");
    
    if (nickname != "")
		loc = gen(NickServ::GetLang(nickname));
	else if (!config->Getvalue("language").empty())
		loc = gen(config->Getvalue("language"));
	else
		loc = gen("en");
	
	std::string msg = translate(fmt).str(loc);

	char buffer[512];
	va_list args;
	va_start (args, fmt);
	vsnprintf (buffer, 512, msg.c_str(), args);
	va_end (args);
	return buffer;
}

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}


std::string Utils::GetEmoji(const std::string &ip) {
  CURL *curl;
  std::string readBuffer;
  curl = curl_easy_init();
  if(curl) {
	std::string url = "http://api.ipstack.com/" + ip + "?access_key=" + config->Getvalue("ipstack") + "&fields=location&output=json";
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    std::string error;
	Json res = Json::array { Json::parse(readBuffer, error) };
	if (error.empty())
		return res[0]["location"]["country_flag_emoji"].string_value();
	else
		return error;
  } else
		curl_easy_cleanup(curl);
	return "No Data";
}
