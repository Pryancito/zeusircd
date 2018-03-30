#include "server.h"
#include "mainframe.h"
#include "oper.h"
#include "session.h"
#include "db.h"
#include "services.h"

CloneMap mClones;
ServerSet Servers;

Server::Server(boost::asio::io_service& io_service, std::string s_ip, int s_port, bool s_ssl, bool s_ipv6)
:   mAcceptor(io_service, tcp::endpoint(boost::asio::ip::address::from_string(s_ip), s_port)), ip(s_ip), port(s_port), ssl(s_ssl), ipv6(s_ipv6)
{
    mAcceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    mAcceptor.listen();
}

void Server::start() { 
	std::cout << "[Server] Server started "  << std::endl;
	startAccept(); 
}

void Server::startAccept() {
	boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
	ctx.use_certificate_file("server.pem", boost::asio::ssl::context::pem);
	ctx.use_private_key_file("server.key", boost::asio::ssl::context::pem);
	ctx.use_tmp_dh_file("dh.pem");
	if (ssl == true) {
		Session::pointer newclient = Session::create_ssl(mAcceptor.get_io_service(), ctx);
		newclient->ssl = true;
		mAcceptor.async_accept(newclient->socket_ssl().lowest_layer(),
                           boost::bind(&Server::handleAccept,   this,   newclient,  boost::asio::placeholders::error));
	} else {
		Session::pointer newclient = Session::create(mAcceptor.get_io_service(), ctx);
		newclient->ssl = false;
		mAcceptor.async_accept(newclient->socket(),
                           boost::bind(&Server::handleAccept,   this,   newclient,  boost::asio::placeholders::error));
	}
}

void Server::handleAccept(Session::pointer newclient, const boost::system::error_code& error) {
	if (CheckClone(newclient->ip()) == true) {
		newclient->close();
		startAccept();
	} if (ssl == true) {
		boost::system::error_code ec;
		newclient->socket_ssl().handshake(boost::asio::ssl::stream_base::server, ec);		
		if (ec) {
			newclient->close();
			std::cout << "SSL ERROR: " << ec << std::endl;
			startAccept();
		}
	} if(error) {
        newclient->close();
    } else {
        CloneUP(newclient->ip());
        newclient->start();
    }
    startAccept();
}

bool Server::CheckClone(const std::string ip) {
	if (mClones.count(ip)) {
		if (mClones[ip] >= (unsigned int )stoi(config->Getvalue("clones")))
			return true;
		else
			return false;
	} else
		return false;
}

void Server::CloneUP(const std::string ip) {
    if (mClones.count(ip) > 0)
		mClones[ip] += 1;
	else
		mClones[ip] = 1;
}

bool Server::HUBExiste() {
	if (config->Getvalue("serverName") == config->Getvalue("hub"))
		return true;
	ServerSet::iterator it = Servers.begin();
    for (; it != Servers.end(); ++it)
		if ((*it)->name() == config->Getvalue("hub"))
			return true;
	return false;
}

void Servidor::Connect(std::string ipaddr, std::string port) {
	bool ssl = false;
	int puerto;
	Oper oper;
	if (Servidor::IsAServer(ipaddr) == false) {
		oper.GlobOPs("Servidor " + ipaddr + " no esta en el fichero de configuracion.");
		return;
	}
	if (port[0] == '+') {
		puerto = (int ) stoi(port.substr(1));
		ssl = true;
	} else
		puerto = (int ) stoi(port);
		
	boost::system::error_code error;
	boost::asio::ip::tcp::endpoint Endpoint(
		boost::asio::ip::address::from_string(ipaddr), puerto);
	boost::asio::io_service io_service;
	boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
	ctx.use_certificate_file("server.pem", boost::asio::ssl::context::pem);
	ctx.use_private_key_file("server.key", boost::asio::ssl::context::pem);
	ctx.use_tmp_dh_file("dh.pem");
	if (ssl == true) {
		Servidor::pointer newserver = Servidor::servidor_ssl(io_service, ctx);
		newserver->ssl = true;
		newserver->socket_ssl().lowest_layer().connect(Endpoint, error);
		if (!error)
			newserver->Procesar();
		else
			oper.GlobOPs("No se ha podido conectar con el servidor: " + ipaddr + " Error: " + error.message());
	} else {
		Servidor::pointer newserver = Servidor::servidor(io_service, ctx);
		newserver->ssl = false;
		newserver->socket().connect(Endpoint, error);
		if (!error)
			newserver->Procesar();
		else
			oper.GlobOPs("No se ha podido conectar con el servidor: " + ipaddr + " Error: " + error.message());
	}
}

void Server::servidor() {
	boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
	ctx.use_certificate_file("server.pem", boost::asio::ssl::context::pem);
	ctx.use_private_key_file("server.key", boost::asio::ssl::context::pem);
	ctx.use_tmp_dh_file("dh.pem");
	Oper oper;
	if (ssl == true) {
		Servidor::pointer newserver = Servidor::servidor_ssl(mAcceptor.get_io_service(), ctx);
		newserver->ssl = true;
		mAcceptor.accept(newserver->socket_ssl().lowest_layer());
		if (Servidor::IsAServer(newserver->socket_ssl().lowest_layer().remote_endpoint().address().to_string()) == false) {
			oper.GlobOPs("Intento de conexion de :" + newserver->socket_ssl().lowest_layer().remote_endpoint().address().to_string() + " - No se encontro en la configuracion.");
			newserver->close();
			return;
		} else if (Servidor::IsConected(newserver->socket_ssl().lowest_layer().remote_endpoint().address().to_string()) == 1) {
			oper.GlobOPs("El servidor " + newserver->socket_ssl().lowest_layer().remote_endpoint().address().to_string() + " ya existe, se ha ignorado el intento de conexion.");
			newserver->close();
			return;
		}
		newserver->Procesar();
	} else {
		Servidor::pointer newserver = Servidor::servidor(mAcceptor.get_io_service(), ctx);
		newserver->ssl = false;
		mAcceptor.accept(newserver->socket());
		if (Servidor::IsAServer(newserver->socket().remote_endpoint().address().to_string()) == false) {
			oper.GlobOPs("Intento de conexion de :" + newserver->socket().remote_endpoint().address().to_string() + " - No se encontro en la configuracion.");
			newserver->close();
			return;
		} else if (Servidor::IsConected(newserver->socket().remote_endpoint().address().to_string()) == 1) {
			oper.GlobOPs("El servidor " + newserver->socket().remote_endpoint().address().to_string() + " ya existe, se ha ignorado el intento de conexion.");
			newserver->close();
			return;
		}
		newserver->Procesar();
	}
	servidor();
}

void Servidor::Procesar() {
	boost::asio::streambuf buffer;
	boost::system::error_code error;

	if (ssl == true) {
		boost::system::error_code ec;
		this->socket_ssl().handshake(boost::asio::ssl::stream_base::server, ec);		
		if (ec) {
			this->close();
			std::cout << "SSL ERROR: " << ec << std::endl;
			return;
		}
		ipaddress = this->socket_ssl().lowest_layer().remote_endpoint().address().to_string();
	} else {
		ipaddress = this->socket().remote_endpoint().address().to_string();
	}
	Oper oper;
	oper.GlobOPs("Conexion con " + ipaddress + " correcta. Sincronizando ....");
	Servidor::SendBurst(this);
	oper.GlobOPs("Fin de sincronizacion de " + ipaddress);

	do {
		if (ssl == false)
			boost::asio::read_until(this->socket(), buffer, '\n', error);
		else
			boost::asio::read_until(this->socket_ssl(), buffer, '\n', error);

        if (error)
        	break;
        
    	std::istream str(&buffer);
		std::string data; 
		std::getline(str, data);

		//Server::Message(server, data);

	} while (this->socket().is_open() || this->socket_ssl().lowest_layer().is_open());
	//Server::SQUIT(s);
	return;	
}

boost::asio::ip::tcp::socket& Servidor::socket() { return mSocket; }

boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& Servidor::socket_ssl() { return mSSL; }

void Servidor::close() {
	if (ssl == true) {
		mSSL.lowest_layer().cancel();
	} else {
		mSocket.cancel();
	}
}

std::string Servidor::name() {
	return nombre;
}

std::string Servidor::ip() {
	return ipaddress;
}

std::string Servidores::name() {
	return nombre;
}

std::string Servidores::ip() {
	return ipaddress;
}

bool Servidor::IsAServer (std::string ip) {
	for (unsigned int i = 0; config->Getvalue("link["+std::to_string(i)+"]ip").length() > 0; i++)
		if (config->Getvalue("link["+std::to_string(i)+"]ip") == ip)
				return true;
	return false;
}

bool Servidor::IsConected (std::string ip) {
	ServerSet::iterator it = Servers.begin();
    for(; it != Servers.end(); ++it)
		if ((*it)->ip() == ip)
			return true;
	return false;
}

void Servidor::send(const std::string& message) {
	boost::system::error_code ignored_error;
	if (ssl == true)
		boost::asio::write(mSSL, boost::asio::buffer(message), boost::asio::transfer_all(), ignored_error);
	else
		boost::asio::write(mSocket, boost::asio::buffer(message), boost::asio::transfer_all(), ignored_error);
}	

Servidores::Servidores(std::string name, std::string ip) : nombre(name), ipaddress(ip) {}

void Servidor::addServer(std::string name, std::string ip) {
	Servidores *server = new Servidores(name, ip);
	Servers.insert(server);
}

void Servidor::SendBurst (Servidor *server) {
	server->send("HUB " + config->Getvalue("hub") + config->EOFServer);
	
	std::string version = "VERSION ";
	if (DB::GetLastRecord() != "") {
		version.append(DB::GetLastRecord() + config->EOFServer);
	} else {
		version.append("0" + config->EOFServer);
	}
	server->send(version);
	ServerSet::iterator it5 = Servers.begin();
    for(; it5 != Servers.end(); ++it5) {
		std::string servidor = "SERVER " + (*it5)->name() + " " + (*it5)->ip() + " 0";
		for (unsigned int i = 0; i < (*it5)->connected.size(); i++) {
			servidor.append(" ");
			servidor.append((*it5)->connected[i]);
		}
		servidor.append(config->EOFServer);
		server->send(servidor);
	}
	UserMap usermap = Mainframe::instance()->users();
	UserMap::iterator it = usermap.begin();
	for (; it != usermap.end(); ++it) {
		std::string modos = "+";
		if (it->second->getMode('r') == true)
			modos.append("r");
		if (it->second->getMode('z') == true)
			modos.append("z");
		if (it->second->getMode('w') == true)
			modos.append("w");
		if (it->second->getMode('o') == true)
			modos.append("o");
		server->send("SNICK " + it->second->nick() + " " + it->second->ident() + " " + it->second->host() + " " + it->second->cloak() + " " + std::to_string(it->second->GetLogin()) + " " + it->second->server() + " " + modos + config->EOFServer);
	}
	ChannelMap channels = Mainframe::instance()->channels();
	ChannelMap::iterator it2 = channels.begin();
	for (; it2 != channels.end(); ++it2) {
		BanSet bans = it2->second->bans();
		BanSet::iterator it3 = bans.begin();
		for (; it3 != bans.end(); ++it3)
			server->send("SBAN " + it2->first + " " + (*it3)->mask() + " " + (*it3)->whois() + " " + std::to_string((*it3)->time()) + config->EOFServer);
		UserSet users = it2->second->users();
		UserSet::iterator it4 = users.begin();
		for (; it4 != users.end(); ++it4) {
			std::string mode;
			if (it2->second->isOperator(*it4) == true)
				mode.append("+o");
			else if (it2->second->isHalfOperator(*it4) == true)
				mode.append("+h");
			else if (it2->second->isVoice(*it4) == true)
				mode.append("+v");
			else
				mode.append("+x");
			server->send("SJOIN " + (*it4)->nick() + " " + it2->first + " " + mode + config->EOFServer);
		}
	}
	//for (auto it = memos.begin(); it != memos.end(); it++)
		//s->Write("MEMO " + (*it)->sender + " " + (*it)->receptor + " " + boost::to_string((*it)->time) + " " + (*it)->mensaje + config->EOFServer);

	OperServ::ApplyGlines();
}
