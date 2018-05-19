#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>

#include "defines.h"
#include "session.h"

using boost::asio::ip::tcp;
typedef std::map<std::string, unsigned int> 	CloneMap;

class Server {
    public:
   
        Server(boost::asio::io_service& io_service, std::string s_ip, int s_port, bool s_ssl, bool s_ipv6);
        Server ();

        void    	start();
        bool 		CheckClone(const std::string ip);
        static void CloneUP(const std::string ip);
        static void ThrottleUP(const std::string ip);
        static bool	HUBExiste();
        void 		servidor();
		bool 		CheckDNSBL(const std::string ip);
		bool 		CheckThrottle(const std::string ip);
    private:
        
        void    startAccept();
        void    handleAccept(Session::pointer newclient, const boost::system::error_code& error);
        void	handle_handshake(Session::pointer newclient, const boost::system::error_code& error);
        void	Procesar(Session* server);
        tcp::acceptor       mAcceptor;
        std::string ip;
        int port;
        bool ssl;
        bool ipv6;
};
