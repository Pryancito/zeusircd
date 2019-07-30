#include "ZeusBaseClass.h"

auto LogPrinter = [](const std::string& strLogMsg) { std::cout << strLogMsg << std::endl;  };

void PublicSock::Listen(std::string ip, std::string port)
{	
	for (;;) {
		CTCPServer socket(LogPrinter, ip, port);
		auto newclient = std::make_shared<LocalUser>(socket);
		socket.Listen(newclient->ConnectedClient);
		std::thread t([newclient] { newclient->start(); });
		t.detach();
	}
}

void PublicSock::SSListen(std::string ip, std::string port)
{
	CTCPSSLServer socket(LogPrinter, ip, port);
	
	auto newclient = std::make_shared<LocalSSLUser>(socket);
	
	std::future<void> futListen = std::async(std::launch::async,
		[&]
        {
			socket.Listen(newclient->ConnectedClient);
			std::thread t([newclient] { newclient->start(); });
			t.detach();
			SSListen(ip, port);
		});
}

void LocalUser::start()
{
	UserSock::Receive();
}

void LocalSSLUser::start()
{
	UserSSLSock::Receive();
}

void UserSock::Receive()
{
	bool quit = false;
	CTCPServer::SetRcvTimeout(ConnectedClient, 1000);
	do {
		char buffer[1024] = {};
		CTCPServer::Receive(ConnectedClient, buffer, 1023);
		std::string message = buffer;
		message = message.substr(0, message.find("\r\n"));
		std::cout << "Parse: " << message << std::endl;
		if (strcasecmp(message.c_str(), "QUIT") < 1)
			quit = true;
	} while (quit == false);
	Close();
}

void UserSock::Send(const std::string message)
{
	CTCPServer::Send(ConnectedClient, message);
}

void UserSock::Close()
{
	CTCPServer::Disconnect(ConnectedClient);
}

void UserSSLSock::Receive()
{
	char szRcvBuffer[1024] = {};
	CTCPSSLServer::Receive(ConnectedClient, szRcvBuffer, 1023);
	std::string message = szRcvBuffer;
	
	std::vector<std::string> str;
	size_t pos;
	while ((pos = message.find("\n")) != std::string::npos) {
		str.push_back(message.substr(0, pos));
		size_t pos2;
		while ((pos2 = message.find("\r")) != std::string::npos) {
			str.push_back(message.substr(0, pos2));
		}
	}
	
	for (unsigned int i = 0; i < str.size(); i++)
		if (str[i].length() > 0)
			//Parser::Parse(str[i], this);
			std::cout << "ParseSSL: " << str[i] << std::endl;
	Receive();
}

void UserSSLSock::Send(const std::string message)
{
	CTCPSSLServer::Send(ConnectedClient, message);
}

void UserSSLSock::Close()
{
	CTCPSSLServer::Disconnect(ConnectedClient);
}
