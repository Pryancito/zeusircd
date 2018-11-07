#include "utils.h"
#include "services.h"
#include "config.h"

#include <stdarg.h>
#include <boost/locale.hpp>

using namespace boost::locale;
using namespace std;
    
bool Utils::isnumber(std::string cadena)
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
	else if (config->Getvalue("language").empty() == true || (config->Getvalue("language") != "es" && config->Getvalue("language") != "en"))
		loc = gen("en");
	else
		loc = gen(config->Getvalue("language"));
	
	std::string msg = translate(fmt).str(loc);

	char buffer[1024];
	va_list args;
	va_start (args, fmt);
	vsnprintf (buffer, 1024, msg.c_str(), args);
	va_end (args);
	return buffer;
}
