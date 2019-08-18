#include "ZeusBaseClass.h"
#include "Server.h"
#include "pool.h"

auto LogPrinter = [](const std::string& strLogMsg) { };
std::mutex quit_mtx;
extern bool exiting;

#ifdef LINUX
Poller_sigio p;
#else
Poller_kqueue p;
#endif

ThreadPool pool(std::thread::hardware_concurrency());

void PublicSock::Listen(std::string ip, std::string port)
{
	p.init();
	for (;;) {
		CTCPServer socket(LogPrinter, ip, port);
		auto newclient = std::make_shared<PlainUser>(socket);
		socket.Listen(newclient->ConnectedClient);
		if (Server::CanConnect(newclient->Server.IP(newclient->ConnectedClient)) == false)
			continue;
		Server::ThrottleUP(newclient->Server.IP(newclient->ConnectedClient));
		newclient->start();
		pool.enqueue([newclient] { newclient->fpool(); });
	}
}

void PublicSock::SSListen(std::string ip, std::string port)
{
	p.init();
	for (;;) {
		CTCPSSLServer socket(LogPrinter, ip, port);
		socket.SetSSLCertFile("server.pem");
		socket.SetSSLKeyFile("server.key");
		auto newclient = std::make_shared<LocalSSLUser>(socket);
		socket.Listen(newclient->ConnectedClient);
		if (Server::CanConnect(newclient->Server.IP(newclient->ConnectedClient)) == false)
			continue;
		Server::ThrottleUP(newclient->Server.IP(newclient->ConnectedClient));
		newclient->start();
		pool.enqueue([newclient] { newclient->fpool(); });
	}
}

void PublicSock::WebListen(std::string ip, std::string port)
{
	LocalWebUser newclient(ip, port);
	newclient.run();
}

void PlainUser::fpool()
{
	while (exiting == false) {
		p.waitAndDispatchEvents(100);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		pool.enqueue([this] { this->PlainUser::fpool(); });
	}
}

void LocalSSLUser::fpool()
{
	while (exiting == false) {
		p.waitAndDispatchEvents(100);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		pool.enqueue([this] { this->LocalSSLUser::fpool(); });
	}
}

void PlainUser::start()
{
	StartTimers(&timers);
	mHost = Server.IP(ConnectedClient);
	p.add(ConnectedClient, cli, POLLIN);
}

void PlainUser::Send(const std::string message)
{
	Server.SetSndTimeout(ConnectedClient, 1000);
	if (Server.Send(ConnectedClient, message + "\r\n") == false)
	{
		quit_mtx.lock();
		Exit();
		quit_mtx.unlock();
		Close();
	}
}

void PlainUser::Close()
{
	quit = true;
	Server.Disconnect(ConnectedClient);
}

void LocalSSLUser::start()
{
	StartTimers(&timers);
	mHost = Server.IP(ConnectedClient);
	p.add(ConnectedClient.m_SockFd, cli, POLLIN);
}

void LocalSSLUser::Send(const std::string message)
{
	Server.SetSndTimeout(ConnectedClient, 1000);
	if (Server.Send(ConnectedClient, message + "\r\n") == false)
	{
		quit_mtx.lock();
		Exit();
		quit_mtx.unlock();
		Close();
	}
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

int PlainUser::notifyPollEvent(Poller::PollEvent *e)
{
	Server.SetRcvTimeout(ConnectedClient, 1000);
	char buffer[1024] = {};
	Server.Receive(ConnectedClient, buffer, 1023, false);
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
			if (quit == true) {
				quit_mtx.lock();
				Exit();
				quit_mtx.unlock();
				Close();
				return -1;
			}
		}
	}
	return 0;
}

int LocalSSLUser::notifyPollEvent(Poller::PollEvent *e)
{
	Server.SetRcvTimeout(ConnectedClient, 1000);
	char buffer[1024] = {};
	Server.Receive(ConnectedClient, buffer, 1023, false);
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
			if (quit == true) {
				quit_mtx.lock();
				Exit();
				quit_mtx.unlock();
				Close();
				return -1;
			}
		}
	}
	return 0;
}
