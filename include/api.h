#ifndef __API__
#define __API__

#include <map>
#include <vector>
#include <string>
#include <set>
#include <boost/regex.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>

using std::map;
using std::string;
using std::set;
using std::vector;

namespace  ourapi
{
class Executor
{   
public:
    enum outputType {
        TYPE_JSON, TYPE_XML   
    };
    Executor();
    bool isreg(struct MHD_Connection *connection, const vector<string>& args, outputType type,  string& response);
    bool registro(struct MHD_Connection *connection, const vector<string>& args, outputType type, string& response);
    bool drop(struct MHD_Connection *connection, const vector<string>& args, outputType type, string& response);
    bool auth(struct MHD_Connection *connection, const vector<string>& args, outputType type, string& response);
    bool online(struct MHD_Connection *connection, const vector<string>& args, outputType type, string& response);
    bool pass(struct MHD_Connection *connection, const vector<string>& args, outputType type, string& response);
    bool email(struct MHD_Connection *connection, const vector<string>& args, outputType type, string& response);
private:
    void _generateOutput(void *data, outputType type, string& output);

};
class api
{
public:
    api();
    bool executeAPI(struct MHD_Connection *connection, const string& url, const map<string, string>& argval, 
            string& response);
	static void http();
private:
    Executor _executor;
    bool _validate(const void*  data);
    bool _executeAPI(struct MHD_Connection *connection, const string& url, const vector<string>& argvals, 
            Executor::outputType type, string& response);
    void _getInvalidResponse(string& response);
    map<string, set<string> > _apiparams;
};
class StrUtil
{
public:
    static void eraseWhiteSpace(string& input);
    static void eraseAllChars(string& input, const char *chars_to_erase);
    static void splitString(const string& input, const char* delims, 
            vector<string>& tokens);
};

}

#endif
