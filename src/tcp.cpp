#include "include.h"
#include <chrono>

using namespace std;

void init_openssl()
{ 
    SSL_load_error_strings();	
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl()
{
    EVP_cleanup();
}

SSL_CTX *create_context()
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_server_method();

    ctx = SSL_CTX_new(method);
    if (!ctx) {
	perror("Unable to create SSL context");
	ERR_print_errors_fp(stderr);
	exit(EXIT_FAILURE);
    }

    return ctx;
}

void configure_context(SSL_CTX *ctx)
{
    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(ctx, "server.pem", SSL_FILETYPE_PEM) < 0) {
        ERR_print_errors_fp(stderr);
	exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) < 0 ) {
        ERR_print_errors_fp(stderr);
	exit(EXIT_FAILURE);
    }
}

TCPAcceptor::TCPAcceptor(int port, const char* address) 
    : m_lsd(0), m_port(port), m_address(address), m_listening(false) {} 

TCPAcceptor::~TCPAcceptor()
{
    if (m_lsd > 0) {
        close(m_lsd);
    }
}

int TCPAcceptor::start()
{
    if (m_listening == true) {
        return 0;
    }

    m_lsd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address2;

    memset(&address2, 0, sizeof(address2));
    address2.sin_family = PF_INET;
    address2.sin_port = htons(m_port);
    inet_pton(PF_INET, m_address.c_str(), &(address2.sin_addr));
    
    int optval = 1;
    setsockopt(m_lsd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval); 
    
    int result = bind(m_lsd, (struct sockaddr*)&address2, sizeof(address2));
    if (result != 0) {
        perror("bind() failed");
        return result;
    }
    
    result = listen(m_lsd, 5);
    if (result != 0) {
        perror("listen() failed");
        return result;
    }
    m_listening = true;
    return result;
}

TCPStream* TCPAcceptor::accept(bool m_SSL) 
{
	SSL_CTX *ctx;
	SSL *ssl;
	int err;
    if (m_listening == false) {
        return NULL;
    }

    init_openssl();
    ctx = create_context();

    configure_context(ctx);
    struct sockaddr_in address;
    socklen_t len = sizeof(address);
    memset(&address, 0, sizeof(address));
    int sd = ::accept(m_lsd, (struct sockaddr*)&address, &len);
    if (sd < 0) {
        perror("accept() failed");
        return NULL;
    }
    ssl = SSL_new(ctx);
    err = SSL_accept(ssl);
    if ( err <= 0 ) {    /* do SSL-protocol accept */
        printf("%d\n",err);
		ERR_print_errors_fp(stderr);
	}
    SSL_set_fd(ssl, sd);
    if (m_SSL == 1)
	    return new TCPStream(sd, &address, ssl);
	else
		return new TCPStream(sd, &address);
}

TCPStream* TCPConnector::connect(const char* server, int port)
{
    struct sockaddr_in address;

    memset (&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (resolveHostName(server, &(address.sin_addr)) != 0 ) {
        inet_pton(PF_INET, server, &(address.sin_addr));        
    } 

    // Create and connect the socket, bail if we fail in either case
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        perror("socket() failed");
        return NULL;
    }
    if (::connect(sd, (struct sockaddr*)&address, sizeof(address)) != 0) {
        perror("connect() failed");
        close(sd);
        return NULL;
    }
    return new TCPStream(sd, &address);
}

TCPStream* TCPConnector::connect(const char* server, int port, int timeout)
{
    if (timeout == 0) return connect(server, port);
    
    struct sockaddr_in address;

    memset (&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (resolveHostName(server, &(address.sin_addr)) != 0 ) {
        inet_pton(PF_INET, server, &(address.sin_addr));        
    }     

    long arg;
    fd_set sdset;
    struct timeval tv;
    socklen_t len;
    int result = -1, valopt, sd = socket(AF_INET, SOCK_STREAM, 0);
    
    // Bail if we fail to create the socket
    if (sd < 0) {
        perror("socket() failed");
        return NULL;
    }    

    // Set socket to non-blocking
    arg = fcntl(sd, F_GETFL, NULL);
    arg |= O_NONBLOCK;
    fcntl(sd, F_SETFL, arg);
    
    // Connect with time limit
    string message;
    if ((result = ::connect(sd, (struct sockaddr *)&address, sizeof(address))) < 0) 
    {
        if (errno == EINPROGRESS)
        {
            tv.tv_sec = timeout;
            tv.tv_usec = 0;
            FD_ZERO(&sdset);
            FD_SET(sd, &sdset);
            int s = -1;
            do {
                s = select(sd + 1, NULL, &sdset, NULL, &tv);
            } while (s == -1 && errno == EINTR);
            if (s > 0)
            {
                len = sizeof(int);
                getsockopt(sd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &len);
                if (valopt) {
                    fprintf(stderr, "connect() error %d - %s\n", valopt, strerror(valopt));
                }
                // connection established
                else result = 0;
            }
            else fprintf(stderr, "connect() timed out\n");
        }
        else fprintf(stderr, "connect() error %d - %s\n", errno, strerror(errno));
    }

    // Return socket to blocking mode 
    arg = fcntl(sd, F_GETFL, NULL);
    arg &= (~O_NONBLOCK);
    fcntl(sd, F_SETFL, arg);

    // Create stream object if connected
    if (result == -1) return NULL;
    return new TCPStream(sd, &address);
}

int TCPConnector::resolveHostName(const char* hostname, struct in_addr* addr) 
{
    struct addrinfo *res;
  
    int result = getaddrinfo (hostname, NULL, NULL, &res);
    if (result == 0) {
        memcpy(addr, &((struct sockaddr_in *) res->ai_addr)->sin_addr, sizeof(struct in_addr));
        freeaddrinfo(res);
    }
    return result;
}

TCPStream::TCPStream(int sd, struct sockaddr_in* address, SSL *ssl) : m_sd(sd), m_ssl(ssl) {
    char ip[50];
    inet_ntop(PF_INET, (struct in_addr*)&(address->sin_addr.s_addr), ip, sizeof(ip)-1);
    m_peerIP = ip;
    m_peerPort = ntohs(address->sin_port);
    m_SSL = 1;
}

TCPStream::TCPStream(int sd, struct sockaddr_in* address) : m_sd(sd) {
    char ip[50];
    inet_ntop(PF_INET, (struct in_addr*)&(address->sin_addr.s_addr), ip, sizeof(ip)-1);
    m_peerIP = ip;
    m_peerPort = ntohs(address->sin_port);
}

TCPStream::TCPStream(int sd, struct sockaddr_in6* address, SSL *ssl) : m_sd(sd), m_ssl(ssl) {
    char ip[100];
    inet_ntop(AF_INET6, &(address->sin6_addr),ip, sizeof(ip)-1);
    m_peerIP = ip;
    m_peerPort = ntohs(address->sin6_port);
    m_SSL = 1;
}

TCPStream::TCPStream(int sd, struct sockaddr_in6* address) : m_sd(sd) {
    char ip[100];
    inet_ntop(AF_INET6, &(address->sin6_addr),ip, sizeof(ip)-1);
    m_peerIP = ip;
    m_peerPort = ntohs(address->sin6_port);
}

TCPStream::~TCPStream()
{
	if (server->IsAServerTCP(this) == 1) {
		string nombre = server->GetServerTCP(this);
		int id = server->GetIDS(this);
		server->SendToAllServers("SQUIT " + nombre);
		for (unsigned int j = 0; j < datos->servers[id]->connected.size(); j++) {
			server->SQUITByServer(datos->servers[id]->connected[j]);
			datos->DeleteServer(datos->servers[id]->connected[j]);
		}
		server->SQUITByServer(nombre);
		datos->DeleteServer(nombre);
	} else {
	    int sID = datos->BuscarIDStream(this);
		chan->PropagarQUIT(this);
		server->SendToAllServers("QUIT " + nick->GetNick(sID));
		datos->BorrarNick(this);
	}
	shutdown(m_sd, 2);
}

ssize_t TCPStream::send(const char* buffer, size_t len) 
{
	if (m_SSL == 1)
		return SSL_write(m_ssl, buffer, len);
	else
	    return write(m_sd, buffer, len);
}

ssize_t TCPStream::receive(char* buffer, size_t len, int timeout) 
{
    if (timeout <= 0) {
		if (m_SSL == 1) return SSL_read(m_ssl, buffer, len);
		else return read(m_sd, buffer, len);
	}
    if (waitForReadEvent(timeout) == true)
    {
        if (m_SSL == 1) return SSL_read(m_ssl, buffer, len);
		else return read(m_sd, buffer, len);
    }
    return connectionTimedOut;

}

string TCPStream::getPeerIP() 
{
    return m_peerIP;
}

int TCPStream::getPeerPort() 
{
    return m_peerPort;
}

int TCPStream::getPeerSocket() 
{
   	return m_sd;
}

bool TCPStream::getSSL()
{
	return m_SSL;
}

bool TCPStream::waitForReadEvent(int timeout)
{
    fd_set sdset;
    struct timeval tv;

    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    FD_ZERO(&sdset);
    FD_SET(m_sd, &sdset);
    if (select(m_sd+1, &sdset, NULL, NULL, &tv) > 0)
    {
        return true;
    }
    return false;
}

TCPAcceptor6::TCPAcceptor6(int port, const char* address) 
    : m_lsd(0), m_port(port), m_address(address), m_listening(false) {} 

TCPAcceptor6::~TCPAcceptor6()
{
    if (m_lsd > 0) {
        close(m_lsd);
    }
}

int TCPAcceptor6::start()
{
    if (m_listening == true) {
        return 0;
    }
    m_lsd = socket(PF_INET6, SOCK_STREAM, 0);
    struct sockaddr_in6 address2;

    memset(&address2, 0, sizeof(address2));
    address2.sin6_family = AF_INET6;
    address2.sin6_port = htons(m_port);
    inet_pton(AF_INET6, m_address.c_str(), &(address2.sin6_addr));
    
    int optval = 1;
    setsockopt(m_lsd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval); 
    
    int result = bind(m_lsd, (struct sockaddr*)&address2, sizeof(address2));
    if (result != 0) {
        perror("bind() failed");
        return result;
    }
    
    result = listen(m_lsd, 5);
    if (result != 0) {
        perror("listen() failed");
        return result;
    }
    m_listening = true;
    return result;
}

TCPStream* TCPAcceptor6::accept(bool m_SSL) 
{
	SSL_CTX *ctx;
	SSL *ssl;
	int err;
    if (m_listening == false) {
        return NULL;
    }

    init_openssl();
    ctx = create_context();

    configure_context(ctx);
    struct sockaddr_in6 address;
    socklen_t len = sizeof(address);
    memset(&address, 0, sizeof(address));
    int sd = ::accept(m_lsd, (struct sockaddr*)&address, &len);
    if (sd < 0) {
        perror("accept() failed");
        return NULL;
    }
    ssl = SSL_new(ctx);
    err = SSL_accept(ssl);
    if ( err <= 0 ) {    /* do SSL-protocol accept */
        printf("%d\n",err);
		ERR_print_errors_fp(stderr);
	}
    SSL_set_fd(ssl, sd);
    if (m_SSL == 1)
	    return new TCPStream(sd, &address, ssl);
	else
		return new TCPStream(sd, &address);
}
