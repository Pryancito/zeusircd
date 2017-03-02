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
    int     m_sd;
    string  m_peerIP;
    int     m_peerPort;
	
  public:
    friend class TCPAcceptor;
    friend class TCPConnector;

    ~TCPStream();

    ssize_t send(const char* buffer, size_t len);
    ssize_t receive(char* buffer, size_t len, int timeout=0);

    string getPeerIP();
    int    getPeerPort();
	int    getPeerSocket();

    enum {
        connectionClosed = 0,
        connectionReset = -1,
        connectionTimedOut = -2
    };

  private:
    bool waitForReadEvent(int timeout);
    
    TCPStream(int sd, struct sockaddr_in* address);
    TCPStream();
    TCPStream(const TCPStream& stream);
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
    TCPStream* accept();

  private:
    TCPAcceptor() {}
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
