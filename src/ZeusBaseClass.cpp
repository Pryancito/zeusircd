#include "ZeusBaseClass.h"
#include "Server.h"

#include <thread>
#include <mutex>

auto LogPrinter = [](const std::string& strLogMsg) { std::cout << strLogMsg << std::endl;  };
std::mutex quit_mtx;

void PublicSock::Listen(std::string ip, std::string port)
{	
	for (;;) {
		CTCPServer socket(LogPrinter, ip, port);
		auto newclient = std::make_shared<PlainUser>(socket);
		socket.Listen(newclient->ConnectedClient);
		Server::ThrottleUP(newclient->IP(newclient->ConnectedClient));
		std::thread t([newclient] { newclient->start(); });
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
		Server::ThrottleUP(newclient->IP(newclient->ConnectedClient));
		std::thread t([newclient] { newclient->start(); });
		t.detach();
	}
}

void PublicSock::WebListen(std::string ip, std::string port)
{
	LocalWebUser newclient(ip, port);
	newclient.run();
}

bool IsConnected(int Socket)
{        
	int optval;
	socklen_t optlen = sizeof(optval);
	
	int res = getsockopt(Socket,SOL_SOCKET,SO_ERROR,&optval, &optlen);
	
	if(optval==0 && res==0) return true;
	
	return false;
}

void PlainUser::start()
{
	if (Server::CanConnect(IP(ConnectedClient)) == false) {
		Close();
		return;
	}
	StartTimers(&timers);
	CTCPServer::SetRcvTimeout(ConnectedClient, 10000);
	mHost = IP(ConnectedClient);
	do {
		char buffer[1024] = {};
		int received = CTCPServer::Receive(ConnectedClient, buffer, 1023, false);
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
	} while (quit == false && IsConnected(ConnectedClient) == true);
	quit_mtx.lock();
	Exit();
	quit_mtx.unlock();
	if (IsConnected(ConnectedClient) == true)
		Close();
}

void PlainUser::Send(const std::string message)
{
	CTCPServer::Send(ConnectedClient, message + "\r\n");
}

void PlainUser::Close()
{
	CTCPServer::Disconnect(ConnectedClient);
	quit = true;
}

void LocalSSLUser::start()
{
	if (Server::CanConnect(IP(ConnectedClient)) == false) {
		Close();
		return;
	}
	StartTimers(&timers);
	CTCPSSLServer::SetRcvTimeout(ConnectedClient, 10000);
	mHost = IP(ConnectedClient);
	do {
		char buffer[1024] = {};
		int received = CTCPSSLServer::Receive(ConnectedClient, buffer, 1023, false);
		if (received <= 0)
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
	} while (quit == false && IsConnected(ConnectedClient.m_SockFd) == true);
	quit_mtx.lock();
	Exit();
	quit_mtx.unlock();
	if (IsConnected(ConnectedClient.m_SockFd) == true)
		Close();
}

void LocalSSLUser::Send(const std::string message)
{
	CTCPSSLServer::Send(ConnectedClient, message + "\r\n");
}

void LocalSSLUser::Close()
{
	CTCPSSLServer::Disconnect(ConnectedClient);
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
