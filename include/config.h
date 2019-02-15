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
#pragma once

#include <cstring>
#include <map>
#include <vector>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <fstream>

class Config
{
        public:
                std::map <std::string, std::string> conf;
                std::string version = "Zeus-4.0.9";
                std::string file = "zeus.conf";
                std::string EOFMessage = "\r\n";
                std::string EOFServer = "\n";

        void Cargar ();
        void Procesa (std::string linea);
        void Configura (std::string dato, const std::string &valor);
        std::string Getvalue (std::string dato);
        void 	MainSocket(std::string ip, int port, bool ssl, bool ipv6);
        void 	ServerSocket(std::string ip, int port, bool ssl, bool ipv6);
        void 	WebSocket(std::string ip, int port, bool ssl, bool ipv6);
};

extern Config *config;

namespace Response
{
    //RFC 1459 Error codes
    namespace Error
    {
        enum error_code
        {
            ERR_NOSUCHNICK = 401,
            ERR_NOSUCHSERVER,
            ERR_NOSUCHCHANNEL = 403,
            ERR_CANNOTSENDTOCHAN,
            ERR_TOOMANYCHANNELS,
            ERR_WASNOSUCHNICK,
            ERR_TOOMANYTARGETS,
            ERR_NOORIGIN = 409,
            ERR_NORECIPIENT = 411,
            ERR_NOTEXTTOSEND,
            ERR_NOTOPLEVEL,
            ERR_WILDTOPLEVEL,
            ERR_UNKNOWNCOMMAND = 421,
            ERR_NOMOTD,
            ERR_NOADMININFO,
            ERR_FILEERROR,
            ERR_NONICKNAMEGIVEN = 431,
            ERR_ERRONEUSNICKNAME,
            ERR_NICKNAMEINUSE,
            ERR_NICKCOLLISION = 436,
            ERR_USERNOTINCHANNEL = 441,
            ERR_NOTONCHANNEL,
            ERR_USERONCHANNEL,
            ERR_NOLOGIN,
            ERR_SUMMONDISABLED,
            ERR_USERSDISABLED,
            ERR_NOTREGISTERED = 451,
            ERR_NEEDMOREPARAMS = 461,
            ERR_ALREADYREGISTRED,
            ERR_NOPERMFORHOST,
            ERR_PASSWDMISMATCH,
            ERR_YOUREBANNEDCREEP,
            ERR_KEYSET = 467,
            ERR_CHANNELISFULL = 471,
            ERR_UNKNOWNMODE,
            ERR_INVITEONLYCHAN,
            ERR_BANNEDFROMCHAN,
            ERR_BADCHANNELKEY,
            ERR_NOPRIVILEGES = 481
        };
    }

    //Enum with RFC 1459 Reply codes   
    namespace Reply
    {
        enum rpl_code
        {
            RPL_NONE = 300,
            RPL_AWAY,
            RPL_USERHOST,
            RPL_ISON,
            RPL_ENDOFWHO = 315,
            RPL_LISTSTART = 321,
            RPL_LIST,
            RPL_LISTEND,
            RPL_NOTOPIC = 331,
            RPL_TOPIC = 332,
            RPL_WHOREPLY = 352,
            RPL_NAMREPLY = 353,
            RPL_ENDOFNAMES = 366,
            RPL_YOUREOPER = 381
        };
    }
}
