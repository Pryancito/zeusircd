#include <microhttpd.h>             
#include <iostream>                 
#include <map>                      
#include <string>                   
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "api.h"
#include "include.h"
#include "sha256.h"

using std::map;
using std::string;
using std::vector;
using boost::property_tree::ptree;
using std::make_pair;
using boost::lexical_cast;
using boost::bad_lexical_cast;
using boost::format;
using boost::regex_search;
using boost::match_default;
using boost::match_results;
using boost::regex;
using boost::property_tree::read_json;
using boost::property_tree::write_json;

using namespace ourapi;

int shouldNotExit = 1;

#define PAGE "<html><head><title>Error</title></head><body>Bad data</body></html>"
 
static int send_bad_response( struct MHD_Connection *connection)
{                                                               
    static char *bad_response = (char *)PAGE;                   
    int bad_response_len = strlen(bad_response);                
    int ret;                                                    
    struct MHD_Response *response;                              
 
    response = MHD_create_response_from_buffer ( bad_response_len,
                bad_response,MHD_RESPMEM_PERSISTENT);             
    if (response == 0){                                           
        return MHD_NO;                                            
    }                                                             
    ret = MHD_queue_response (connection, MHD_HTTP_OK, response); 
    MHD_destroy_response (response);                              
    return ret;                                                   
}                                                                 

static int get_url_args(void *cls, MHD_ValueKind kind,
                    const char *key , const char* value)
{                                                       
    map<string, string> * url_args = static_cast<map<string, string> *>(cls);
 
    if (url_args->find(key) == url_args->end()) {
         if (!value)                                   
             (*url_args)[key] = "";                    
         else                                         
            (*url_args)[key] = value;                  
    }                                                  
    return MHD_YES;                                    
 
}
                 
static int url_handler (void *cls,
    struct MHD_Connection *connection,
    const char *url,                  
    const char *method,               
    const char *version,              
    const char *upload_data, size_t *upload_data_size, void **ptr)
{                                                                 
    static int aptr;                                              
    char *me;                                                     
    const char *typexml = "xml";                                  
    const char *typejson = "json";                                
    const char *type = typejson;                                  

    struct MHD_Response *response;
    int ret;                      
    map<string, string> url_args;
    map<string, string>:: iterator  it;
    ourapi::api callapi;                     
    string respdata;                         
 
    if (&aptr != *ptr) {
        *ptr = &aptr;   
        return MHD_YES; 
    }                   
 
 
    if (MHD_get_connection_values (connection, MHD_GET_ARGUMENT_KIND, 
                           get_url_args, &url_args) < 0) {         
        return send_bad_response(connection);                         
 
    }
 
    callapi.executeAPI(connection, url, url_args, respdata);

    *ptr = 0;                  /* reset when done */
    MHD_lookup_connection_value (connection, MHD_GET_ARGUMENT_KIND, "q");
    me = (char *)malloc (respdata.size() + 1);                                 
    if (me == 0)                                                               
        return MHD_NO;                                                         
    strncpy(me, respdata.c_str(), respdata.size() + 1);                        
    response = MHD_create_response_from_buffer (strlen (me), me,               
                                              MHD_RESPMEM_MUST_FREE);          
    if (response == 0){                                                        
        free (me);                                                             
        return MHD_NO;                                                         
    }                                                                          
 
    it = url_args.find("type");
    if (it != url_args.end() && strcasecmp(it->second.c_str(), "xml") == 0)
        type = typexml;                                                       
 
    MHD_add_response_header(response, "Content-Type", "text");
    MHD_add_response_header(response, "OurHeader", type);     
 
    ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    MHD_destroy_response (response);                             
    return ret;                                                  
}                                                                
 
void handle_term(int signo)
{                          
    shouldNotExit = 0;     
}                          
 
void api::http()
{
    struct MHD_Daemon *d;

    d = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY | MHD_USE_DEBUG | MHD_USE_POLL, 8000, 0, 0, &url_handler, (void *)PAGE, MHD_OPTION_END);
    if (d == NULL){
        return;
    }
    while(shouldNotExit) {
        sleep(1);
    }
    MHD_stop_daemon (d);
    return;
}
 
struct validate_data
{                   
    string api;     
    set <string>* params; 
};                              
 
api::api()
{         
    set<string> params;
    string isregparams[] = {"nick"}; 
    string regparams[] = {"nick", "pass" };
    string droparams[] = {"nick"};
	string authparams[] = {"nick", "pass"};
	string onlineparams[] = {"nick"};
	string passparams[] = {"nick", "pass"};
	string emailparams[] = {"nick", "email"};
 
    _apiparams["/isreg"] =  set<string>(isregparams, isregparams + 1);
    _apiparams["/register"] = set<string>(regparams, regparams  + 2);
    _apiparams["/drop"] = set<string>(droparams, droparams + 1);
	_apiparams["/auth"] = set<string>(authparams, authparams  + 2);
	_apiparams["/online"] =  set<string>(onlineparams, onlineparams + 1);
	_apiparams["/pass"] = set<string>(passparams, passparams  + 2);
	_apiparams["/email"] = set<string>(emailparams, emailparams  + 2);
}                                                                                          
 
bool api::executeAPI(struct MHD_Connection *connection, const string& url, const map<string, string>& argvals, string& response)
{                                                                                                  
    // Ignore all the args except the "fields" param                                               
    validate_data vdata ;                                                                          
    vdata.api = url;                                                                               
    Executor::outputType type = Executor::TYPE_JSON;                                               
    vector<string> params;                                                                   
    set<string> uniqueparams;
	vector<string> parametros;                                                             
    map<string,string>::const_iterator it1 = argvals.find("data");                         
 
    if (it1 != argvals.end()) {
        string prms = it1->second;
        StrUtil::eraseWhiteSpace(prms);
        StrUtil::splitString(prms, ",=", params);   
    }                                              
    BOOST_FOREACH( string pr, params ) {           
        uniqueparams.insert(pr);
		parametros.push_back(pr);                
    }
    
	vdata.params = &uniqueparams;                           
    
    it1 = argvals.find("type");
    if (it1 != argvals.end()){ 
        const string outputtype = it1->second;
        if (strcasecmp(outputtype.c_str(), "xml") == 0 ) {
            type = Executor::TYPE_XML;                    
        }                                                 
    }                                                     
 
    return _executeAPI(connection, url, parametros, type, response);
}                                                         
 
bool api::_executeAPI(struct MHD_Connection *connection, const string& url, const vector<string>& argvals, 
        Executor::outputType type, string& response)                       
{                                                                          
    bool ret = false;
    if (url == "/isreg")
        ret = _executor.isreg(connection, argvals, type,  response);
    if (url == "/register")
        ret = _executor.registro(connection, argvals, type, response);
    if (url == "/drop")
        ret = _executor.drop(connection, argvals, type, response);
    if (url == "/auth")
        ret = _executor.auth(connection, argvals, type, response);
    if (url == "/online")
        ret = _executor.online(connection, argvals, type, response);
    if (url == "/pass")
        ret = _executor.pass(connection, argvals, type, response);
    if (url == "/email")
        ret = _executor.email(connection, argvals, type, response);
 
    return ret;
}
 
bool api::_validate(const void *data)
{
    const validate_data *vdata = static_cast<const validate_data *>(data );
    map<string, set<string> > ::iterator it =  _apiparams.find(vdata->api);

    it = _apiparams.find(vdata->api);

    if ( it == _apiparams.end()){
        return false;
    }
    set<string>::iterator it2 = vdata->params->begin();
    while (it2 != vdata->params->end()) {
        if (it->second.find(*it2) == it->second.end()) 
            return false;
        ++it2;
    }

    return true;
}

void api::_getInvalidResponse(string& response)
{
    ptree pt;
	pt.put ("status", "ERROR");
	pt.put ("message", "Error en los datos.");
	std::ostringstream buf; 
	write_json (buf, pt, false);
	std::string json = buf.str();
	response = json;
}

Executor::Executor()
{                   
}                   
 
bool Executor::isreg(struct MHD_Connection *connection, const vector<string>& args, outputType type, 
        string& response)                                               
{
	if (args.size() < 1) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", "Error al introducir el nick o el canal.");
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (args[0][0] == '#') {
		if (checkchan(args[0]) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", "El canal contiene caracteres no validos.");
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		} else if (ChanServ::IsRegistered(args[0]) == 1) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", "El canal " + args[0] + " ya esta registrado.");
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		} else {
			ptree pt;
			pt.put ("status", "OK");
			pt.put ("message", "El canal " + args[0] + " no esta registrado.");
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return true;
		}
	} else {
		if (checknick(args[0]) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", "El canal " + args[0] + " no esta registrado.");
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			response = "El nick contiene caracteres no validos.";
			return false;
		} else if (NickServ::IsRegistered(args[0]) == 1) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", "El nick " + args[0] + " ya esta registrado.");
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		} else {
			ptree pt;
			pt.put ("status", "OK");
			pt.put ("message", "El nick " + args[0] + " no esta registrado.");
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return true;
		}
	}
    return false;
}
 
bool Executor::registro(struct MHD_Connection *connection, const vector<string>& args, outputType type, 
        string& response)                                               
{
	if (args.size() < 2) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", "Error al introducir los datos.");
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (checknick(args[0]) == false) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", "El Nick contiene caracteres no validos.");
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (NickServ::IsRegistered(args[0]) == 1) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", "El nick " + args[0] + " ya esta registrado.");
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (Servidor::HUBExiste() == 0) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", "El HUB no existe, las BDs estan en modo de solo lectura.");
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (args[1].find(":") != std::string::npos || args[1].find("!") != std::string::npos || args[1].find(";") != std::string::npos || args[1].find("'") != std::string::npos) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", "El password contiene caracteres no validos (!:;').");
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (User::FindNick(args[0]) == true) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", "El nick " + args[0] + " esta en uso, no puede ser registrado.");
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else {
		string sql = "INSERT INTO NICKS VALUES ('" + args[0] + "', '" + sha256(args[1]) + "', '', '', '',  " + boost::to_string(time(0)) + ", " + boost::to_string(time(0)) + ");";
		if (DB::SQLiteNoReturn(sql) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", "El nick " + args[0] + " no ha sido registrado. Contacte con algun iRCop.");
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		}
		sql = "DB " + DB::GenerateID() + " " + sql;
		DB::AlmacenaDB(sql);
		Servidor::SendToAllServers(sql);
		sql = "INSERT INTO OPTIONS VALUES ('" + args[0] + "', 0, 0, 0, 0, 0);";
		if (DB::SQLiteNoReturn(sql) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", "El nick " + args[0] + " no ha sido registrado. Contacte con algun iRCop.");
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		}
		sql = "DB " + DB::GenerateID() + " " + sql;
		DB::AlmacenaDB(sql);
		Servidor::SendToAllServers(sql);
		ptree pt;
		pt.put ("status", "OK");
		pt.put ("message", "El nick " + args[0] + " ha sido registrado.");
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return true;
	}
    return false;                                                                                        
}                                                                                                       
 
bool Executor::drop(struct MHD_Connection *connection, const vector<string>& args, outputType type, 
        string& response)                                              
{                     
	if (args.size() < 1) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", "Error al introducir el nick o el canal.");
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (args[0][0] == '#') {
		if (checkchan(args[0]) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", "El canal contiene caracteres no validos.");
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		} else if (ChanServ::IsRegistered(args[0]) == 0) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", "El canal " + args[0] + " no esta registrado.");
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		} else {
			string sql = "DELETE FROM CANALES WHERE NOMBRE='" + args[0] + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				ptree pt;
				pt.put ("status", "ERROR");
				pt.put ("message", "El canal " + args[0] + " no ha sido borrado. Contacte con un iRCop.");
				std::ostringstream buf; 
				write_json (buf, pt, false);
				std::string json = buf.str();
				response = json;
				return false;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::SendToAllServers(sql);
			sql = "DELETE FROM ACCESS WHERE CANAL='" + args[0] + "' COLLATE NOCASE;";
			DB::SQLiteNoReturn(sql);
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::SendToAllServers(sql);
			sql = "DELETE FROM AKICK WHERE CANAL='" + args[0] + "' COLLATE NOCASE;";
			DB::SQLiteNoReturn(sql);
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::SendToAllServers(sql);
			for (Chan *canal = canales.first(); canal != NULL; canal = canales.next(canal))
				if (boost::iequals(canal->GetNombre(), args[0], loc)) {
					if (canal != NULL) {
						if (canal->Tiene_Modo('r') == true) {
							canal->Fijar_Modo('r', false);
							Chan::PropagarMODE("CHaN!*@*", "", args[0], 'r', 0, 1);
						}
					}
				}
			ptree pt;
			pt.put ("status", "OK");
			pt.put ("message", "El canal " + args[0] + " ha sido borrado.");
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return true;
		}
	} else {
		if (checknick(args[0]) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", "El nick contiene caracteres no validos.");
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		} else if (NickServ::IsRegistered(args[0]) == 0) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", "El nick " + args[0] + " no esta registrado.");
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		} else if (User::FindNick(args[0]) == true) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", "El nick " + args[0] + " esta en uso, no puede ser borrado.");
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		} else {
			string sql = "DELETE FROM NICKS WHERE NICKNAME='" + args[0] + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				ptree pt;
				pt.put ("status", "ERROR");
				pt.put ("message", "El nick " + args[0] + " no ha sido borrado. Contacte con un iRCop.");
				std::ostringstream buf; 
				write_json (buf, pt, false);
				std::string json = buf.str();
				response = json;
				return false;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::SendToAllServers(sql);
			sql = "DELETE FROM OPTIONS WHERE NICKNAME='" + args[0] + "' COLLATE NOCASE;";
			DB::SQLiteNoReturn(sql);
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::SendToAllServers(sql);
			sql = "DELETE FROM ACCESS WHERE USUARIO='" + args[0] + "' COLLATE NOCASE;";
			DB::SQLiteNoReturn(sql);
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::SendToAllServers(sql);
			ptree pt;
			pt.put ("status", "OK");
			pt.put ("message", "El nick " + args[0] + " ha sido borrado.");
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return true;
		}
	}
    return true;
}

bool Executor::auth(struct MHD_Connection *connection, const vector<string>& args, outputType type, 
        string& response)                                               
{
	if (args.size() < 2) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", "Error al introducir el nick o la contraseña.");
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (checknick(args[0]) == false) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", "El nick contiene caracteres no validos.");
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (args[1].find(":") != std::string::npos || args[1].find("!") != std::string::npos || args[1].find(";") != std::string::npos || args[1].find("'") != std::string::npos) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", "El password contiene caracteres no validos (!:;').");
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (NickServ::IsRegistered(args[0]) == 0) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", "El nick " + args[0] + " no esta registrado.");
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (NickServ::Login(args[0], args[1]) == 1) {
		ptree pt;
		pt.put ("status", "OK");
		pt.put ("message", "Identificacion correcta.");
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return true;
	} else {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", "Error en la identificacion.");
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	}
    return false;
}

bool Executor::online(struct MHD_Connection *connection, const vector<string>& args, outputType type, 
        string& response)                                               
{
	if (checknick(args[0]) == false) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", "El nick contiene caracteres no validos.");
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (User::FindNick(args[0]) == true) {
		ptree pt;
		pt.put ("status", "OK");
		pt.put ("message", "El nick " + args[0] + " esta conectado.");
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return true;
	} else {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", "El nick " + args[0] + " no esta conectado.");
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	}
}

bool Executor::pass(struct MHD_Connection *connection, const vector<string>& args, outputType type, 
        string& response)                                               
{
	if (checknick(args[0]) == false) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", "El nick contiene caracteres no validos.");
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (args[1].find(":") != std::string::npos || args[1].find("!") != std::string::npos || args[1].find(";") != std::string::npos || args[1].find("'") != std::string::npos) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", "El password contiene caracteres no validos (!:;').");
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (NickServ::IsRegistered(args[0]) == 0) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", "El nick " + args[0] + " no esta registrado.");
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else {
		string sql = "UPDATE NICKS SET PASS=" + boost::to_string(args[1]) + " WHERE NICKNAME='" + args[0] + "' COLLATE NOCASE;";
		if (DB::SQLiteNoReturn(sql) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", "La password de el nick " + args[0] + " no ha sido cambiada. Contacte con un IRCop.");
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		}
		sql = "DB " + DB::GenerateID() + " " + sql;
		DB::AlmacenaDB(sql);
		Servidor::SendToAllServers(sql);
		ptree pt;
		pt.put ("status", "OK");
		pt.put ("message", "La password del nick " + args[0] + " ha sido cambiada a: " + args[1] + ".");
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return true;
	}
}

bool Executor::email(struct MHD_Connection *connection, const vector<string>& args, outputType type, 
        string& response)                                               
{
	if (checknick(args[0]) == false) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", "El nick contiene caracteres no validos.");
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (args[1].find("@") == std::string::npos || args[1].find(".") == std::string::npos) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", "La direccion de correo parece no ser valida.");
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (NickServ::IsRegistered(args[0]) == 0) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", "El nick " + args[0] + " no esta registrado.");
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else {
		string sql = "UPDATE NICKS SET EMAIL=" + boost::to_string(args[1]) + " WHERE NICKNAME='" + args[0] + "' COLLATE NOCASE;";
		if (DB::SQLiteNoReturn(sql) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", "El e-mail del nick " + args[0] + " no ha sido cambiado. Contacte con un IRCop.");
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		}
		sql = "DB " + DB::GenerateID() + " " + sql;
		DB::AlmacenaDB(sql);
		Servidor::SendToAllServers(sql);
		ptree pt;
		pt.put ("status", "OK");
		pt.put ("message", "El e-mail del nick " + args[0] + " ha sido cambiado.");
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return true;
	}
}

void Executor::_generateOutput(void *data, outputType type, string& output)
{
    std::ostringstream ostr;
    ptree *pt = (ptree *) data;
    if (TYPE_JSON == type)
        write_json(ostr, *pt);
    else if (TYPE_XML == type)
        write_xml(ostr, *pt);
 
    output = ostr.str();
}

void StrUtil::eraseWhiteSpace(string& val)
{
    boost::erase_all(val," ");
    boost::erase_all(val,"\n");
    boost::erase_all(val,"\t");
    boost::erase_all(val,"\r");
}

void StrUtil::eraseAllChars(string& val, const char *chars_to_erase)
{
    if (!chars_to_erase)
        return;

    vector<string> tokens;
    boost::split(tokens, val, boost::is_any_of(chars_to_erase), boost::token_compress_on ); 
    val = "";
    BOOST_FOREACH(string a, tokens) {
        val += a;
    }
}

void StrUtil::splitString(const string& input, const char* delims,
        vector <string>& tokens)
{
    string temp = input;
    boost::trim_if(temp, boost::is_any_of(delims));
    boost::split( tokens, temp, boost::is_any_of(delims), boost::token_compress_on ); 
}
