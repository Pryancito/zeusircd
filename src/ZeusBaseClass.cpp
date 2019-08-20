#include "ZeusBaseClass.h"
#include "Server.h"
#include "pool.h"

auto LogPrinter = [](const std::string& strLogMsg) { };
std::mutex quit_mtx;
extern bool exiting;

#ifdef LINUX
Poller_epoll p;
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
		std::thread t([newclient] { newclient->fpool(); });
		t.detach();
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
		std::thread t([newclient] { newclient->fpool(); });
		t.detach();
	}
}

void PublicSock::WebListen(std::string ip, std::string port)
{
	LocalWebUser newclient(ip, port);
	newclient.run();
}

void PlainUser::fpool()
{
	while (exiting == false)
		p.waitAndDispatchEvents(10);
}

void LocalSSLUser::fpool()
{
	while (exiting == false)
		p.waitAndDispatchEvents(10);
}

void PlainUser::start()
{
	StartTimers(&timers);
	mHost = Server.IP(ConnectedClient);
	Server.SetSndTimeout(ConnectedClient, 5000);
	p.add(ConnectedClient, cli, POLLIN);
}

void PlainUser::Send(const std::string message)
{
	if (ConnectedClient >= 0) {
		if (Server.Send(ConnectedClient, message + "\r\n") == false)
		{
			quit_mtx.lock();
			Exit();
			quit_mtx.unlock();
			Server.Disconnect(ConnectedClient);
			p.del(ConnectedClient);
			return;
		}
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
	Server.SetSndTimeout(ConnectedClient, 5000);
	p.add(ConnectedClient.m_SockFd, cli, POLLIN);
}

void LocalSSLUser::Send(const std::string message)
{
	if (ConnectedClient.m_SockFd >= 0) {
		if (Server.Send(ConnectedClient, message + "\r\n") == false)
		{
			quit_mtx.lock();
			Exit();
			quit_mtx.unlock();
			Server.Disconnect(ConnectedClient);
			p.del(ConnectedClient.m_SockFd);
			return;
		}
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
	char buffer[1024] = {};
	if (Server.Receive(ConnectedClient, buffer, 1023, false) == false)
	{
		quit_mtx.lock();
		Exit();
		quit_mtx.unlock();
		Server.Disconnect(ConnectedClient);
		p.del(ConnectedClient);
		return -1;
	}
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
			Parse(str[i]);
			if (LocalUser::quit == true) {
				quit_mtx.lock();
				Exit();
				quit_mtx.unlock();
				Server.Disconnect(ConnectedClient);
				p.del(ConnectedClient);
				return -1;
			}
		}
	}
	return 0;
}

int LocalSSLUser::notifyPollEvent(Poller::PollEvent *e)
{
	char buffer[1024] = {};
	if (Server.Receive(ConnectedClient, buffer, 1023, false) == false)
	{
		quit_mtx.lock();
		Exit();
		quit_mtx.unlock();
		Server.Disconnect(ConnectedClient);
		p.del(ConnectedClient.m_SockFd);
		return -1;
	}
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
			Parse(str[i]);
			if (LocalUser::quit == true) {
				quit_mtx.lock();
				Exit();
				quit_mtx.unlock();
				Server.Disconnect(ConnectedClient);
				p.del(ConnectedClient.m_SockFd);
				return -1;
			}
		}
	}
	return 0;
}
