#ifndef TCP_H
#define TCP_H

#include <string>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>

using namespace std;

class TCPStream
{
  public:
    int     m_sd;
    string  m_peerIP;
    int     m_peerPort;
    bool	m_SSL = 0;
    SSL		*m_ssl;
  	string	cgiirc;
  	
    friend class TCPAcceptor;
    friend class TCPConnector;

    ~TCPStream();

    ssize_t send(const char* buffer, size_t len);
    ssize_t receive(char* buffer, size_t len, int timeout=0);

    string getPeerIP();
    int    getPeerPort();
	int    getPeerSocket();
	bool   getSSL();

    enum {
        connectionClosed = 0,
        connectionReset = -1,
        connectionTimedOut = -2
    };

    bool waitForReadEvent(int timeout);
    
    TCPStream(int sd, struct sockaddr_in* address, SSL *ssl);
    TCPStream(int sd, struct sockaddr_in* address);
    TCPStream(int sd, struct sockaddr_in6* address, SSL *ssl);
    TCPStream(int sd, struct sockaddr_in6* address);
    TCPStream();
};

class TCPAcceptor
{
    int    m_lsd;
    int    m_port;
    string m_address;
    bool   m_listening;
    
  public:
    TCPAcceptor(int port, const char* address="");
    ~TCPAcceptor();

    int        start();
    TCPStream* accept(bool m_SSL);

  private:
    TCPAcceptor() {}
};

class TCPAcceptor6
{
    int    m_lsd;
    int    m_port;
    string m_address;
    bool   m_listening;
    
  public:
    TCPAcceptor6(int port, const char* address="");
    ~TCPAcceptor6();

    int        start();
    TCPStream* accept(bool m_SSL);

  private:
    TCPAcceptor6() {}
};

class TCPConnector
{
  public:
    TCPStream* connect(const char* server, int port);
    TCPStream* connect(const char* server, int port, int timeout);
    
  private:
    int resolveHostName(const char* host, struct in_addr* addr);
};

#endif
