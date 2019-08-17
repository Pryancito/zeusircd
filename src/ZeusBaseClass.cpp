#include "ZeusBaseClass.h"
#include "Server.h"

auto LogPrinter = [](const std::string& strLogMsg) { };
std::mutex quit_mtx;

void PublicSock::Listen(std::string ip, std::string port)
{
	for (;;) {
		CTCPServer socket(LogPrinter, ip, port);
		auto newclient = std::make_shared<PlainUser>(socket);
		socket.Listen(newclient->ConnectedClient);
		Server::ThrottleUP(newclient->Server.IP(newclient->ConnectedClient));
		std::thread t([newclient]() { newclient->start(); });
		t.detach();
	}
}

void PublicSock::SSListen(std::string ip, std::string port)
{
	for (;;) {
		CTCPSSLServer socket(LogPrinter, ip, port);
		socket.SetSSLCertFile("server.pem");
		socket.SetSSLKeyFile("server.key");
		auto newclient = std::make_shared<LocalSSLUser>(socket);
		socket.Listen(newclient->ConnectedClient);
		Server::ThrottleUP(newclient->Server.IP(newclient->ConnectedClient));
		std::thread t([newclient] { newclient->start(); });
		t.detach();
	}
}

void PublicSock::WebListen(std::string ip, std::string port)
{
	LocalWebUser newclient(ip, port);
	newclient.run();
}

void PlainUser::start()
{
	if (Server::CanConnect(Server.IP(ConnectedClient)) == false) {
		Close();
		return;
	}
	StartTimers(&timers);
	Server.SetSndTimeout(ConnectedClient, 10000);
	Server.SetRcvTimeout(ConnectedClient, 10000);
	mHost = Server.IP(ConnectedClient);
	do {
		char buffer[1024] = {};
		int received = Server.Receive(ConnectedClient, buffer, 1023, false);
		if (received == false)
			break;
		std::string message = buffer;
		std::vector<std::string> str;
		size_t pos;
		while ((pos = message.find("\r\n")) != std::string::npos) {
			str.push_back(message.substr(0, pos));
			message.erase(0, pos + 2);
		} if (str.empty()) {
			while ((pos = message.find("\n")) != std::string::npos) {
				str.push_back(message.substr(0, pos));
				message.erase(0, pos + 1);
			}
		}
		
		for (unsigned int i = 0; i < str.size(); i++) {
			if (str[i].length() > 0) {
				this->LocalUser::Parse(str[i]);
				if (quit == true)
					break;
			}
		}
	} while (quit == false && ConnectedClient > 0);
	quit_mtx.lock();
	Exit();
	quit_mtx.unlock();
	Server.Disconnect(ConnectedClient);
	return;
}

void PlainUser::Send(const std::string message)
{
	if (Server.Send(ConnectedClient, message + "\r\n") == false)
		PlainUser::Close();
}

void PlainUser::Close()
{
	quit = true;
	Server.Disconnect(ConnectedClient);
}

void LocalSSLUser::start()
{
	if (Server::CanConnect(Server.IP(ConnectedClient)) == false) {
		Close();
		return;
	}
	StartTimers(&timers);
	Server.SetRcvTimeout(ConnectedClient, 10000);
	Server.SetSndTimeout(ConnectedClient, 10000);
	mHost = Server.IP(ConnectedClient);
	do {
		char buffer[1024] = {};
		int received = Server.Receive(ConnectedClient, buffer, 1023, false);
		if (received == false)
			break;
		std::string message = buffer;
		std::vector<std::string> str;
		size_t pos;
		while ((pos = message.find("\r\n")) != std::string::npos) {
			str.push_back(message.substr(0, pos));
			message.erase(0, pos + 2);
		} if (str.empty()) {
			while ((pos = message.find("\n")) != std::string::npos) {
				str.push_back(message.substr(0, pos));
				message.erase(0, pos + 1);
			}
		}
		
		for (unsigned int i = 0; i < str.size(); i++) {
			if (str[i].length() > 0) {
				this->LocalUser::Parse(str[i]);
				if (quit == true)
					break;
			}
		}
	} while (quit == false && ConnectedClient.m_SockFd > 0);
	quit_mtx.lock();
	Exit();
	quit_mtx.unlock();
	Server.Disconnect(ConnectedClient);
	return;
}

void LocalSSLUser::Send(const std::string message)
{
	if (Server.Send(ConnectedClient, message + "\r\n") == false)
		Close();
}

void LocalSSLUser::Close()
{
	quit = true;
	Server.Disconnect(ConnectedClient);
}


LocalWebUser::LocalWebUser( std::string ip, std::string port ) : WebSocketServer( ip, port )
{

}

LocalWebUser::~LocalWebUser( )
{

}

void LocalWebUser::onConnect( int socketId )
{
	if (Server::CanConnect(IP(socketId)) == false) {
		Close();
	} else {
		Server::ThrottleUP(IP(socketId));
		SocketID = socketId;
		StartTimers(&timers);
		websocket = true;
		mHost = IP(socketId);
	}
}

void LocalWebUser::onMessage( int socketID, const string& data )
{
	this->LocalUser::Parse ( data );
}

void LocalWebUser::onDisconnect( int socketID )
{
	quit_mtx.lock();
	Exit();
	quit_mtx.unlock();
}

void LocalWebUser::onError( int socketID, const string& message )
{
	std::cout << "WebSocket ERROR: " << message << std::endl; 
}

void LocalWebUser::Send(const std::string message)
{
	this->WebSocketServer::send( SocketID, message );
}

void LocalWebUser::Close()
{
	this->WebSocketServer::Quit(SocketID);
}
