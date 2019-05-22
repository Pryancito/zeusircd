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
#include <microhttpd.h>
#include <iostream>                 
#include <map>                      
#include <string>                   
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/system/error_code.hpp>
#include <fstream>
#include <regex>

#include "api.h"
#include "parser.h"
#include "sha256.h"
#include "server.h"
#include "db.h"
#include "services.h"
#include "mainframe.h"
#include "utils.h"
#include "base64.h"

#define GC_THREADS
#define GC_ALWAYS_MULTITHREADED
#include <gc_cpp.h>
#include <gc.h>

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

extern ForceMap bForce;

#define PAGE "<html><head><title>Error</title></head><body>Invalid data.</body></html>"
 
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

void api::http()
{
    GC_stack_base sb;
    GC_get_stack_base(&sb);
    GC_register_my_thread(&sb);
    struct MHD_Daemon *d;
	struct sockaddr_in loopback_addr;
	
	memset(&loopback_addr, 0, sizeof(loopback_addr));
    loopback_addr.sin_family = AF_INET;
    loopback_addr.sin_port = htons(8000);
    loopback_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	
    d = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY | MHD_USE_DEBUG | MHD_USE_POLL, 8000, 0, 0, &url_handler, (void *)PAGE, MHD_OPTION_SOCK_ADDR, (struct sockaddr *)(&loopback_addr), MHD_OPTION_END);
    if (d == NULL){
        return;
    }
    while(shouldNotExit) {
        sleep(1);
    }
    MHD_stop_daemon (d);
    GC_unregister_my_thread();
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
	string logparams[] = {"search"};
	string glineparams[] = {"ip"};
 
    _apiparams["/isreg"] =  set<string>(isregparams, isregparams + 1);
    _apiparams["/register"] = set<string>(regparams, regparams  + 2);
    _apiparams["/drop"] = set<string>(droparams, droparams + 1);
	_apiparams["/auth"] = set<string>(authparams, authparams  + 2);
	_apiparams["/online"] =  set<string>(onlineparams, onlineparams + 1);
	_apiparams["/pass"] = set<string>(passparams, passparams  + 2);
	_apiparams["/email"] = set<string>(emailparams, emailparams  + 2);
	_apiparams["/log"] = set<string>(logparams, logparams  + 1);
	_apiparams["/ungline"] = set<string>(glineparams, glineparams  + 1);
	
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
    else if (url == "/register")
        ret = _executor.registro(connection, argvals, type, response);
    else if (url == "/drop")
        ret = _executor.drop(connection, argvals, type, response);
    else if (url == "/auth")
        ret = _executor.auth(connection, argvals, type, response);
    else if (url == "/online")
        ret = _executor.online(connection, argvals, type, response);
    else if (url == "/pass")
        ret = _executor.pass(connection, argvals, type, response);
    else if (url == "/email")
        ret = _executor.email(connection, argvals, type, response);
	else if (url == "/logs")
        ret = _executor.logs(connection, argvals, type, response);
    else if (url == "/ungline")
        ret = _executor.ungline(connection, argvals, type, response);
        
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
	pt.put ("message", Utils::make_string("", "Data Error.").c_str());
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
		pt.put ("message", Utils::make_string("", "Error in nick or channel input.").c_str());
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (args[0][0] == '#') {
		if (Parser::checkchan(args[0]) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The channel contains no-valid characters.").c_str());
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		} else if (ChanServ::IsRegistered(args[0]) == 1) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The channel %s is already registered.", args[0].c_str()).c_str());
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		} else {
			ptree pt;
			pt.put ("status", "OK");
			pt.put ("message", Utils::make_string("", "The channel %s is not registered.", args[0].c_str()).c_str());
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return true;
		}
	} else {
		if (Parser::checknick(args[0]) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The nick contains no-valid characters.").c_str());
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		} else if (NickServ::IsRegistered(args[0]) == 1) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The nick %s is already registered.", args[0].c_str()).c_str());
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		} else {
			ptree pt;
			pt.put ("status", "OK");
			pt.put ("message", Utils::make_string("", "The nick %s is not registered.", args[0].c_str()).c_str());
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
		pt.put ("message", Utils::make_string("", "Error at data input.").c_str());
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (args[0][0] == '#') {
		if (Parser::checkchan(args[0]) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The channel contains no-valid characters.").c_str());
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		} else if (ChanServ::IsRegistered(args[0]) == 1) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The channel %s is already registered.", args[0].c_str()).c_str());
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		} else if (Server::HUBExiste() == 0) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The HUB doesnt exists, DBs are in read-only mode.").c_str());
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		} else if (NickServ::IsRegistered(args[1]) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The nick %s is not registered.", args[1].c_str()).c_str());
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		} else {
			std::string sql = "INSERT INTO CANALES VALUES ('" + args[0] + "', '" + args[1] + "', '+r', '', '" + Base64::Encode(Utils::make_string("", "The channel has been registered.")) + "',  " + std::to_string(time(0)) + ", " + std::to_string(time(0)) + ");";
			if (DB::SQLiteNoReturn(sql) == false) {
				ptree pt;
				pt.put ("status", "ERROR");
				pt.put ("message", Utils::make_string("", "The channel %s cannot be registered. Please contact with an iRCop.", args[0].c_str()).c_str());
				std::ostringstream buf; 
				write_json (buf, pt, false);
				std::string json = buf.str();
				response = json;
				return false;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			if (config->Getvalue("cluster") == "false")
				Servidor::sendall(sql);
			sql = "INSERT INTO CMODES (CANAL) VALUES ('" + args[0] + "');";
			DB::SQLiteNoReturn(sql);
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			if (config->Getvalue("cluster") == "false")
				Servidor::sendall(sql);
			Channel* chan = Mainframe::instance()->getChannelByName(args[0]);
			User* target = Mainframe::instance()->getUserByName(args[1]);
			if (chan) {
				if (chan->getMode('r') == false) {
					chan->setMode('r', true);
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +r" + config->EOFMessage);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +r");
				}
				if (target) {
					if (chan->hasUser(target)) {
						if (!chan->isOperator(target)) {
							chan->giveOperator(target);
							chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +o " + target->nick() + config->EOFMessage);
							Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +o " + target->nick());
						}
					}
				}
			}
			ptree pt;
			pt.put ("status", "OK");
			pt.put ("message", Utils::make_string("", "The channel %s has been registered.", args[0].c_str()).c_str());
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return true;
		}
	} else {
		if (Parser::checknick(args[0]) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The nick contains no-valid characters.").c_str());
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		} else if (NickServ::IsRegistered(args[0]) == 1) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The nick %s is already registered.", args[0].c_str()).c_str());
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		} else if (Server::HUBExiste() == 0) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The HUB doesnt exists, DBs are in read-only mode.").c_str());
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		} else if (DB::EscapeChar(args[1]) == true) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The password contains no valid characters (!:;').").c_str());
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		} else {
			std::string sql = "INSERT INTO NICKS VALUES ('" + args[0] + "', '" + sha256(args[1]) + "', '', '', '',  " + std::to_string(time(0)) + ", " + std::to_string(time(0)) + ");";
			if (DB::SQLiteNoReturn(sql) == false) {
				ptree pt;
				pt.put ("status", "ERROR");
				pt.put ("message", Utils::make_string("", "The nick %s cannot be registered. Please contact with an iRCop.", args[0].c_str()).c_str());
				std::ostringstream buf; 
				write_json (buf, pt, false);
				std::string json = buf.str();
				response = json;
				return false;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			if (config->Getvalue("cluster") == "false")
				Servidor::sendall(sql);
			sql = "INSERT INTO OPTIONS (NICKNAME, LANG) VALUES ('" + args[0] + "', '" + config->Getvalue("language") + "');";
			if (DB::SQLiteNoReturn(sql) == false) {
				ptree pt;
				pt.put ("status", "ERROR");
				pt.put ("message", Utils::make_string("", "The nick %s cannot be registered. Please contact with an iRCop.", args[0].c_str()).c_str());
				std::ostringstream buf; 
				write_json (buf, pt, false);
				std::string json = buf.str();
				response = json;
				return false;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			if (config->Getvalue("cluster") == "false")
				Servidor::sendall(sql);
			ptree pt;
			pt.put ("status", "OK");
			pt.put ("message", Utils::make_string("", "The nick %s has been registered.", args[0].c_str()).c_str());
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			User *user = Mainframe::instance()->getUserByName(args[0]);
			if (user) {
				if (user->getMode('r') == false) {
					if (user->session())
						user->session()->send(":" + config->Getvalue("serverName") + " MODE " + user->nick() + " +r" + config->EOFMessage);
					user->setMode('r', true);
					Servidor::sendall("UMODE " + user->nick() + " +r");
				}
			}
			return true;
		}
	}
    return false;                                                                                        
}                                                                                                       
 
bool Executor::drop(struct MHD_Connection *connection, const vector<string>& args, outputType type, 
        string& response)                                              
{                     
	if (args.size() < 1) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", Utils::make_string("", "Error at data input.").c_str());
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (args[0][0] == '#') {
		if (Parser::checkchan(args[0]) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The channel contains no-valid characters.").c_str());
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		} else if (ChanServ::IsRegistered(args[0]) == 0) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The channel %s is not registered.", args[0].c_str()).c_str());
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		} else {
			string sql = "DELETE FROM CANALES WHERE NOMBRE='" + args[0] + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				ptree pt;
				pt.put ("status", "ERROR");
				pt.put ("message", Utils::make_string("", "The channel %s cannot be deleted. Please contact with an iRCop.", args[0].c_str()).c_str());
				std::ostringstream buf; 
				write_json (buf, pt, false);
				std::string json = buf.str();
				response = json;
				return false;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			if (config->Getvalue("cluster") == "false")
				Servidor::sendall(sql);
			sql = "DELETE FROM ACCESS WHERE CANAL='" + args[0] + "';";
			DB::SQLiteNoReturn(sql);
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			if (config->Getvalue("cluster") == "false")
				Servidor::sendall(sql);
			sql = "DELETE FROM AKICK WHERE CANAL='" + args[0] + "';";
			DB::SQLiteNoReturn(sql);
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			if (config->Getvalue("cluster") == "false")
				Servidor::sendall(sql);
			sql = "DELETE FROM CMODES WHERE CANAL='" + args[0] + "';";
			DB::SQLiteNoReturn(sql);
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			if (config->Getvalue("cluster") == "false")
				Servidor::sendall(sql);
			Channel* chan = Mainframe::instance()->getChannelByName(args[0]);
			if (chan->getMode('r') == true) {
				chan->setMode('r', false);
				chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -r" + config->EOFMessage);
				Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -r");
			}
			ptree pt;
			pt.put ("status", "OK");
			pt.put ("message", Utils::make_string("", "The channel %s has been deleted.", args[0].c_str()).c_str());
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return true;
		}
	} else {
		if (Parser::checknick(args[0]) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The nick contains no-valid characters.").c_str());
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		} else if (NickServ::IsRegistered(args[0]) == 0) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The nick %s is not registered.", args[0].c_str()).c_str());
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		} else if (Mainframe::instance()->getUserByName(args[0])) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The nick %s is used by somebody. Cannot be deleted.", args[0].c_str()).c_str());
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		} else {
			std::string sql = "DELETE FROM NICKS WHERE NICKNAME='" + args[0] + "';";
			if (DB::SQLiteNoReturn(sql) == false) {
				ptree pt;
				pt.put ("status", "ERROR");
				pt.put ("message", Utils::make_string("", "The nick %s cannot be deleted. Please contact with an iRCop.", args[0].c_str()).c_str());
				std::ostringstream buf; 
				write_json (buf, pt, false);
				std::string json = buf.str();
				response = json;
				return false;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			if (config->Getvalue("cluster") == "false")
				Servidor::sendall(sql);
			sql = "DELETE FROM OPTIONS WHERE NICKNAME='" + args[0] + "';";
			DB::SQLiteNoReturn(sql);
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			if (config->Getvalue("cluster") == "false")
				Servidor::sendall(sql);
			sql = "DELETE FROM CANALES WHERE OWNER='" + args[0] + "';";
			DB::SQLiteNoReturn(sql);
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			if (config->Getvalue("cluster") == "false")
				Servidor::sendall(sql);
			sql = "DELETE FROM ACCESS WHERE USUARIO='" + args[0] + "';";
			DB::SQLiteNoReturn(sql);
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			if (config->Getvalue("cluster") == "false")
				Servidor::sendall(sql);
			ptree pt;
			pt.put ("status", "OK");
			pt.put ("message", Utils::make_string("", "The nick %s has been deleted.", args[0].c_str()).c_str());
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
		pt.put ("message", Utils::make_string("", "Error at data input.").c_str());
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (Parser::checknick(args[0]) == false) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", Utils::make_string("", "The nick contains no-valid characters.").c_str());
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (DB::EscapeChar(args[1]) == true) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", Utils::make_string("", "The password contains no valid characters (!:;').").c_str());
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (NickServ::IsRegistered(args[0]) == 0) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", Utils::make_string("", "The nick %s is not registered.", args[0].c_str()).c_str());
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (bForce[args[0]] >= 7) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", Utils::make_string("", "Too much identify attempts for this nick. Try in 1 hour.").c_str());
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (NickServ::Login(args[0], args[1]) == 1) {
		std::string nick = args[0];
		bForce[nick] = 0;
		ptree pt;
		pt.put ("status", "OK");
		pt.put ("message", Utils::make_string("", "Logued in !").c_str());
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return true;
	} else {
		std::string nick = args[0];
		if (bForce.count(nick) > 0)
			bForce[nick] += 1;
		else
			bForce[nick] = 1;
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", Utils::make_string("", "Wrong login").c_str());
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
	if (Parser::checknick(args[0]) == false) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", Utils::make_string("", "The nick contains no-valid characters.").c_str());
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (Mainframe::instance()->getUserByName(args[0])) {
		ptree pt;
		pt.put ("status", "OK");
		pt.put ("message", Utils::make_string("", "The nick %s is online.", args[0].c_str()).c_str());
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return true;
	} else {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", Utils::make_string("", "The nick %s is offline.", args[0].c_str()).c_str());
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
	if (Parser::checknick(args[0]) == false) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", Utils::make_string("", "The nick contains no-valid characters.").c_str());
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (DB::EscapeChar(args[1]) == true) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", Utils::make_string("", "The password contains no valid characters (!:;').").c_str());
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (NickServ::IsRegistered(args[0]) == 0) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", Utils::make_string("", "The nick %s is not registered.", args[0].c_str()).c_str());
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else {
		string sql = "UPDATE NICKS SET PASS='" + sha256(args[1]) + "' WHERE NICKNAME='" + args[0] + "';";
		if (DB::SQLiteNoReturn(sql) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The password for nick %s cannot be changed. Contact with an iRCop.", args[0].c_str()).c_str());
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		}
		sql = "DB " + DB::GenerateID() + " " + sql;
		DB::AlmacenaDB(sql);
		if (config->Getvalue("cluster") == "false")
			Servidor::sendall(sql);
		ptree pt;
		pt.put ("status", "OK");
		pt.put ("message", Utils::make_string("", "The password for nick %s has been changed to: %s", args[0].c_str(), args[1].c_str()).c_str());
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
	if (Parser::checknick(args[0]) == false) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", Utils::make_string("", "The nick contains no-valid characters.").c_str());
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (std::regex_match(args[1], std::regex("^[_a-z0-9-]+(.[_a-z0-9-]+)*@[a-z0-9-]+(.[a-z0-9-]+)*(.[a-z]{2,4})$")) == false) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", Utils::make_string("", "The email seems to be wrong.").c_str());
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else if (NickServ::IsRegistered(args[0]) == 0) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", Utils::make_string("", "The nick %s is not registered.", args[0].c_str()).c_str());
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else {
		string sql = "UPDATE NICKS SET EMAIL=" + args[1] + " WHERE NICKNAME='" + args[0] + "';";
		if (DB::SQLiteNoReturn(sql) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The e-mail for nick %s cannot be changed. Contact with an iRCop.", args[0].c_str()).c_str());
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		}
		sql = "DB " + DB::GenerateID() + " " + sql;
		DB::AlmacenaDB(sql);
		if (config->Getvalue("cluster") == "false")
			Servidor::sendall(sql);
		ptree pt;
		pt.put ("status", "OK");
		pt.put ("message", Utils::make_string("", "The e-mail for nick %s has been changed.", args[0].c_str()).c_str());
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return true;
	}
}

bool Executor::logs(struct MHD_Connection *connection, const vector<string>& args, outputType type, 
        string& response)                                               
{
	std::ifstream fichero("ircd.log");
	std::string linea;
	std::string respuesta;
	do {
		getline(fichero, linea);
		if (!fichero.eof() && Utils::Match(args[0].c_str(), linea.c_str()) == true)
			respuesta.append(linea + "\n");
	} while (!fichero.eof());
	response = respuesta;
	return true;
}

bool Executor::ungline(struct MHD_Connection *connection, const vector<string>& args, outputType type, 
        string& response)                                               
{
	if (OperServ::IsGlined(args[0]) == false) {
		ptree pt;
		pt.put ("status", "ERROR");
		pt.put ("message", Utils::make_string("", "Does not exists GLINE for that IP.").c_str());
		std::ostringstream buf; 
		write_json (buf, pt, false);
		std::string json = buf.str();
		response = json;
		return false;
	} else {
		string sql = "DELETE FROM GLINE WHERE IP='" + args[0] + "';";
		if (DB::SQLiteNoReturn(sql) == false) {
			ptree pt;
			pt.put ("status", "ERROR");
			pt.put ("message", Utils::make_string("", "The GLINE for IP %s cannot be deleted. Please contact with an iRCop.", args[0].c_str()).c_str());
			std::ostringstream buf; 
			write_json (buf, pt, false);
			std::string json = buf.str();
			response = json;
			return false;
		}
		sql = "DB " + DB::GenerateID() + " " + sql;
		DB::AlmacenaDB(sql);
		if (config->Getvalue("cluster") == "false")
			Servidor::sendall(sql);
		
		ptree pt;
		pt.put ("status", "OK");
		pt.put ("message", Utils::make_string("", "The GLINE for IP %s has been deleted.", args[0].c_str()).c_str());
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

void StrUtil::splitString(const string& input, const char* delims,
        vector <string>& tokens)
{
    string temp = input;
    boost::trim_if(temp, boost::is_any_of(delims));
    boost::split( tokens, temp, boost::is_any_of(delims), boost::token_compress_on ); 
}
