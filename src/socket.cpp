#include "include.h"
#include "../src/lista.cpp"
#include "../src/nodes.cpp"
#include <boost/system/system_error.hpp>
#include "sha256.h"

using namespace std;
std::mutex sock_mtx;
List<Socket*> sock;

std::string invertir(const std::string str)
{
    std::string rstr = str;
    std::reverse(rstr.begin(), rstr.end());
    int size = rstr.size();
    int start = 0;
    int end = 0;
    while (end != size + 1) {
        if (rstr[end] == '.' || end == size) {
            std::reverse(rstr.begin() + start, rstr.begin() + end);
            start = end + 1;
        }
        ++end;
    }
    return rstr;
}

std::string BinToHex(const void* raw, size_t l)
{
	static const char hextable[] = "0123456789abcdef";
	const char* data = static_cast<const char*>(raw);
	std::string rv;
	rv.reserve(l * 2);
	for (size_t i = 0; i < l; i++)
	{
		unsigned char c = data[i];
		rv.push_back(hextable[c >> 4]);
		rv.push_back(hextable[c & 0xF]);
	}
	return rv;
}

std::string invertirv6 (const std::string str) {
	struct in6_addr addr;
    inet_pton(AF_INET6,str.c_str(),&addr);
	const unsigned char* ip = addr.s6_addr;
	std::string reversedip;

	std::string buf = BinToHex(ip, 16);
	for (std::string::const_reverse_iterator it = buf.rbegin(); it != buf.rend(); ++it)
	{
		reversedip.push_back(*it);
		reversedip.push_back('.');
	}
	return reversedip;
}

void Socket::Write (const std::string mensaje) {
	boost::system::error_code ignored_error;
	if (this->GetSSL() == 1)
		boost::asio::write(this->GetSSLSocket(), boost::asio::buffer(mensaje, mensaje.length()), boost::asio::transfer_all(), ignored_error);
	else
		boost::asio::write(this->GetSocket(), boost::asio::buffer(mensaje, mensaje.length()), boost::asio::transfer_all(), ignored_error);
	return;
}

void Socket::Close() {
	boost::system::error_code ec;
	if (this->GetSSL() == 1) {
		this->GetSSLSocket().lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
	} else if (this->GetSSL() == 0) {
		this->GetSocket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
	}
	if (ec) {
		cout << "Shutdown Socket error: " << ec << endl;
	}
}

bool Socket::CheckDNSBL(string ip) {
	string ipcliente;
	for (unsigned int i = 0; config->Getvalue("dnsbl["+to_string(i)+"]suffix").length() > 0; i++) {
		if (config->Getvalue("dnsbl["+to_string(i)+"]reverse") == "true") {
			ipcliente = invertir(ip);
		} else {
			ipcliente = ip;
		}
		string hostname = ipcliente + config->Getvalue("dnsbl["+to_string(i)+"]suffix");
		hostent *record = gethostbyname(hostname.c_str());
		if(record != NULL)
		{
			oper->GlobOPs("Alerta DNSBL. " + config->Getvalue("dnsbl["+to_string(i)+"]suffix") + " IP: " + ip + "\r\n");
			return true;
		}
	}
	return false;
}

bool Socket::CheckDNSBL6(string ip) {
	string ipcliente;
	for (unsigned int i = 0; config->Getvalue("dnsbl6["+to_string(i)+"]suffix").length() > 0; i++) {
		if (config->Getvalue("dnsbl6["+to_string(i)+"]reverse") == "true") {
			ipcliente = invertir(ip);
		} else {
			ipcliente = ip;
		}
		string hostname = ipcliente + config->Getvalue("dnsbl6["+to_string(i)+"]suffix");
		hostent *record = gethostbyname(hostname.c_str());
		if(record != NULL)
		{
			oper->GlobOPs("Alerta DNSBL. " + config->Getvalue("dnsbl6["+to_string(i)+"]suffix") + " IP: " + ip + "\r\n");
			return true;
		}
	}
	return false;
}

void Socket::MainSocket () {
    if (is_SSL == 0) {
		boost::asio::io_service io_service;
		boost::asio::ip::tcp::acceptor acceptor(io_service);
    	boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
		boost::asio::ip::tcp::endpoint Endpoint(
		boost::asio::ip::address::from_string(ip), port);
		acceptor.open(Endpoint.protocol());
		acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		acceptor.bind(Endpoint);
		acceptor.listen();
		cout << "client socket iniciado " << ip << "@" << port << " ... OK" << endl;
    	while (1) {
			Socket *s = new Socket(io_service, ctx);
			acceptor.accept(s->GetSocket());
			
			if (server->CheckClone(s->GetSocket().remote_endpoint().address().to_string()) == true) {
				s->Write(":" + config->Getvalue("serverName") + " 223 :Has superado el numero maximo de clones.\r\n");
				s->Close();
				delete s;
				continue;
			}
			
			if (is_IPv6 == 1) {
				if (s->CheckDNSBL6(s->GetSocket().remote_endpoint().address().to_string()) == true) {
					s->Write(":" + config->Getvalue("serverName") + " 223 :Te conectas desde una conexion prohibida.\r\n");
					s->Close();
					delete s;
					continue;
				}
			} else {
				if (s->CheckDNSBL(s->GetSocket().remote_endpoint().address().to_string()) == true) {
					s->Write(":" + config->Getvalue("serverName") + " 223 :Te conectas desde una conexion prohibida.\r\n");
					s->Close();
					delete s;
					continue;
				}
			}

			s->SetSSL(0);
			s->SetIPv6(is_IPv6);
			s->SetTipo(0);
			sock.add(s);
			s->GetSocket().set_option(boost::asio::ip::tcp::no_delay(true));
			s->tw = new boost::thread(boost::bind(&Socket::Cliente, this, s));
			s->tw->detach();
		}			
	} else {
		boost::asio::io_service io_service;
		boost::asio::ip::tcp::acceptor acceptor(io_service);
		boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
	    ctx.use_certificate_file("server.pem", boost::asio::ssl::context::pem);
	    ctx.use_private_key_file("server.key", boost::asio::ssl::context::pem);
	    ctx.use_tmp_dh_file("dh.pem");
		boost::asio::ip::tcp::endpoint Endpoint(
		boost::asio::ip::address::from_string(ip), port);
		acceptor.open(Endpoint.protocol());
		acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		acceptor.bind(Endpoint);
		acceptor.listen();
		cout << "client socket iniciado " << ip << "@" << port << " ... OK" << endl;
    	while (1) {
    		Socket *s = new Socket(io_service, ctx);
			acceptor.accept(s->GetSSLSocket().lowest_layer(), Endpoint);
			
			if (server->CheckClone(s->GetSSLSocket().lowest_layer().remote_endpoint().address().to_string()) == true) {
				s->Write(":" + config->Getvalue("serverName") + " 223 :Has superado el numero maximo de clones.\r\n");
				s->Close();
				delete s;
				continue;
			}
			
			if (is_IPv6 == 1) {
				if (s->CheckDNSBL6(s->GetSSLSocket().lowest_layer().remote_endpoint().address().to_string()) == true) {
					s->Write(":" + config->Getvalue("serverName") + " 223 :Te conectas desde una conexion prohibida.\r\n");
					s->Close();
					delete s;
					continue;
				}
			} else {
				if (s->CheckDNSBL(s->GetSSLSocket().lowest_layer().remote_endpoint().address().to_string()) == true) {
					s->Write(":" + config->Getvalue("serverName") + " 223 :Te conectas desde una conexion prohibida.\r\n");
					s->Close();
					delete s;
					continue;
				}
			}
			s->GetSSLSocket().lowest_layer().set_option(boost::asio::ip::tcp::no_delay(true));
			s->SetSSL(1);
			s->SetIPv6(is_IPv6);
			s->SetTipo(0);
			sock.add(s);
			s->tw = new boost::thread(boost::bind(&Socket::Cliente, this, s));
			s->tw->detach();
		}
	}
}

void Socket::Cliente (Socket *s) {
	boost::asio::streambuf buffer;
	boost::system::error_code error;
	if (s->GetSSL() == 1) {
		boost::system::error_code ec;
		s->GetSSLSocket().handshake(boost::asio::ssl::stream_base::server, ec);		
		if (ec) {
			s->Close();
			delete s;
			cout << "SSL ERROR: " << ec << endl;
			return;
		}
	}
	string id = sha256(std::to_string(rand())).substr(0, 12);
	User *u = new User(s, id);
	u->SetNodo(config->Getvalue("serverName"));
	u->SetLogin(time(0));
	if (s->GetSSL() == 1) {
		string ipe = s->GetSSLSocket().lowest_layer().remote_endpoint().address().to_string();
		string cloak = sha256(ipe).substr(0, 16);
		u->SetCloakIP(cloak);
		u->SetIP(ipe);
	} else {
		string ipe = s->GetSocket().remote_endpoint().address().to_string();
		string cloak = sha256(ipe).substr(0, 16);
		u->SetCloakIP(cloak);
		u->SetIP(ipe);
	}
	users.add(u);
	do {
		fd_set fileDescriptorSet;
        struct timeval timeStruct;
		int nativeSocket;
        // set the timeout to 30 
        timeStruct.tv_sec = 120;
        timeStruct.tv_usec = 0;
        FD_ZERO(&fileDescriptorSet);
        if (s->GetSSL() == false)
        	nativeSocket = s->GetSocket().native();
        else
        	nativeSocket = s->GetSSLSocket().lowest_layer().native();
        	
        FD_SET(nativeSocket,&fileDescriptorSet);
        select(nativeSocket+1,&fileDescriptorSet,NULL,NULL,&timeStruct);
        if(!FD_ISSET(nativeSocket,&fileDescriptorSet))
        	break;
        	
		if (s->GetSSL() == 1)
			boost::asio::read_until(s->GetSSLSocket(), buffer, '\n', error);
		else
			boost::asio::read_until(s->GetSocket(), buffer, '\n', error);
			
		if (error == boost::asio::error::eof)
        	break;
        if (error)
        	break;
        if (s->IsQuit() == true)
        	break;
        	
    	std::istream str(&buffer);
		std::string data; 
		std::getline(str, data);

		if (data.find('\r') != std::string::npos) {
			size_t tam = data.length();
			data.erase(tam-1);
		}
		
		u->ProcesaMensaje(s, data);
			
        if (s->IsQuit() == true)
        	break;

	} while (s->GetSocket().is_open() || s->GetSSLSocket().lowest_layer().is_open());
	user->Quit(u, s);
	return;
}

void Socket::Conectar(string ip, string port) {
	bool ssl = false;
	int puerto;
	if (server->IsAServer(ip) == false) {
		oper->GlobOPs("Servidor " + ip + " no esta en el fichero de configuracion." + "\r\n");
		return;
	}
	if (port[0] == '+') {
		puerto = (int ) stoi(port.substr(1));
		ssl = true;
	} else
		puerto = (int ) stoi(port);
		
	boost::system::error_code error;
	boost::asio::ip::tcp::endpoint Endpoint(
		boost::asio::ip::address::from_string(ip), puerto);
	boost::asio::io_service io_service;
    boost::asio::ip::tcp::socket socket(io_service);
    boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
    if (ssl == true) {
	    Socket *s = new Socket(io_service, ctx);
	    s->GetSSLSocket().lowest_layer().connect(Endpoint, error);
		if (s->GetSSLSocket().lowest_layer().is_open()) {
			s->SetSSL(1);
			s->SetIPv6(0);
			s->SetTipo(1);
			sock.add(s);
			s->GetSSLSocket().lowest_layer().set_option(boost::asio::ip::tcp::no_delay(true));
			s->tw = new boost::thread(boost::bind(&Socket::Servidor, this, s));
			s->tw->detach();
		}
	} else {
	    Socket *s = new Socket(io_service, ctx);
	    s->GetSocket().connect(Endpoint, error);
		if (s->GetSSLSocket().lowest_layer().is_open()) {
			s->SetSSL(0);
			s->SetIPv6(0);
			s->SetTipo(1);
			sock.add(s);
			s->GetSSLSocket().lowest_layer().set_option(boost::asio::ip::tcp::no_delay(true));
			s->tw = new boost::thread(boost::bind(&Socket::Servidor, this, s));
			s->tw->detach();
		}
	}
	return;
}

void Socket::ServerSocket () {
    if (is_SSL == 0) {
    	boost::asio::io_service io_service;
		boost::asio::ip::tcp::acceptor acceptor(io_service);
    	boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
		boost::asio::ip::tcp::endpoint Endpoint(
		boost::asio::ip::address::from_string(ip), port);
		acceptor.open(Endpoint.protocol());
		acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		acceptor.bind(Endpoint);
		acceptor.listen();
		cout << "server socket iniciado " << ip << "@" << port << " ... OK" << endl;
    	while (1) {
			Socket *s = new Socket(io_service, ctx);
			acceptor.accept(s->GetSocket());
			if (server->IsAServer(s->GetSocket().remote_endpoint().address().to_string()) == 0) {
				oper->GlobOPs("Intento de conexion de :" + s->GetSocket().remote_endpoint().address().to_string() + " - No se encontro en la configuracion.");
				s->Close();
				delete s;
				continue;
			} else if (server->IsConected(s->GetSocket().remote_endpoint().address().to_string()) == 1) {
				oper->GlobOPs("El servidor " + s->GetSocket().remote_endpoint().address().to_string() + " ya existe, se ha ignorado el intento de conexion.");
				s->Close();
				delete s;
				continue;
			}
			oper->GlobOPs("Conexion con " + s->GetSocket().remote_endpoint().address().to_string() + " correcta. Sincronizando ....");
			server->SendBurst(s);
			oper->GlobOPs("Fin de sincronizacion de " + s->GetSocket().remote_endpoint().address().to_string());
			s->SetSSL(0);
			sock.add(s);
			s->tw = new boost::thread(boost::bind(&Socket::Servidor, this, s));
			s->tw->detach();
		}			
	} else {
		boost::asio::io_service io_service;
		boost::asio::ip::tcp::acceptor acceptor(io_service);
		boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
	    ctx.use_certificate_file("server.pem", boost::asio::ssl::context::pem);
	    ctx.use_private_key_file("server.key", boost::asio::ssl::context::pem);
	    ctx.use_tmp_dh_file("dh.pem");
		boost::asio::ip::tcp::endpoint Endpoint(
		boost::asio::ip::address::from_string(ip), port);
		acceptor.open(Endpoint.protocol());
		acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		acceptor.bind(Endpoint);
		acceptor.listen();
		cout << "server socket iniciado " << ip << "@" << port << " ... OK" << endl;
    	while (1) {
    		Socket *s = new Socket(io_service, ctx);
			acceptor.accept(s->GetSSLSocket().lowest_layer(), Endpoint);
			if (server->IsAServer(s->GetSSLSocket().lowest_layer().remote_endpoint().address().to_string()) == 0) {
				oper->GlobOPs("Intento de conexion de :" + s->GetSSLSocket().lowest_layer().remote_endpoint().address().to_string() + " - No se encontro en la configuracion.");
				s->Close();
				delete s;
				continue;
			} else if (server->IsConected(s->GetSSLSocket().lowest_layer().remote_endpoint().address().to_string()) == 1) {
				oper->GlobOPs("El servidor " + s->GetSSLSocket().lowest_layer().remote_endpoint().address().to_string() + " ya existe, se ha ignorado el intento de conexion.");
				s->Close();
				delete s;
				continue;
			}
			s->GetSSLSocket().handshake(boost::asio::ssl::stream_base::server);
			oper->GlobOPs("Conexion con " + s->GetSSLSocket().lowest_layer().remote_endpoint().address().to_string() + " correcta. Sincronizando ....");
			server->SendBurst(s);
			oper->GlobOPs("Fin de sincronizacion de " + s->GetSSLSocket().lowest_layer().remote_endpoint().address().to_string());
			s->SetSSL(1);
			sock.add(s);
			s->tw = new boost::thread(boost::bind(&Socket::Servidor, this, s));
			s->tw->detach();
		}
	}
}

void Socket::Servidor (Socket *s) {
	oper->GlobOPs("Conexion con " + s->GetIP() + " correcta. Sincronizando ....");
	server->SendBurst(s);
	oper->GlobOPs("Fin de sincronizacion de " + s->GetIP());
	boost::asio::streambuf buffer;
	boost::system::error_code error;
	
	do {
		boost::asio::read_until(s->GetSocket(), buffer, "||", error);
			
		if (error == boost::asio::error::eof)
        	break;
        if (error)
        	break;
        
    	std::istream str(&buffer);
		std::string data; 
		std::getline(str, data);

		server->ProcesaMensaje(s, data);

        if (s->IsQuit() == true)
        	break;

	} while (s->GetSocket().is_open() || s->GetSSLSocket().lowest_layer().is_open());
	server->SQUIT(s);
	delete s;
	return;
}

void Socket::SetIP(string ipe) {
	ip = ipe;
}

string Socket::GetIP() {
	return ip;
}

void Socket::SetPort(int puerto) {
	port = puerto;
}

int Socket::GetPort() {
	return port;
}

boost::asio::ip::tcp::socket &Socket::GetSocket() {
	return s_socket;
}

boost::asio::ssl::stream<boost::asio::ip::tcp::socket> &Socket::GetSSLSocket() {
	return s_ssl;
}
void Socket::SetSSL(bool ssl) {
	is_SSL = ssl;
}

bool Socket::GetSSL() {
	return is_SSL;
}

void Socket::SetIPv6(bool ipv6) {
	is_IPv6 = ipv6;
}

bool Socket::GetIPv6() {
	return is_IPv6;
}

bool Socket::GetTipo() {
	return tipo;
}

void Socket::SetTipo(bool tipo_) {
	tipo = tipo_;
}

void Socket::Quit() {
	quit = true;
}

bool Socket::IsQuit() {
	return quit;
}
