/** --------------------------------------------------------------------------
 *  WebSocketServer.cpp
 *
 *  Base class that WebSocket implementations must inherit from.  Handles the
 *  client connections and calls the child class callbacks for connection
 *  events like onConnect, onMessage, and onDisconnect.
 *
 *  Author    : Jason Kruse <jason@jasonkruse.com> or @mnisjk
 *  Copyright : 2014
 *  License   : BSD (see LICENSE)
 *  --------------------------------------------------------------------------
 **/

#include <stdlib.h>
#include <string>
#include <sys/time.h>
#include <fcntl.h>
#include <libwebsockets.h>
#include "WebSocketServer.h"

extern bool IsConnected(int Socket);

using namespace std;

// 0 for unlimited
#define MAX_BUFFER_SIZE 2048

// Nasty hack because certain callbacks are statically defined
WebSocketServer *self;

static int callback_main(   struct lws *wsi,
                            enum lws_callback_reasons reason,
                            void *user,
                            void *in,
                            size_t len )
{
    int fd;
    switch( reason ) {
        case LWS_CALLBACK_ESTABLISHED:
            self->onConnectWrapper( lws_get_socket_fd( wsi ) );
            lws_callback_on_writable( wsi );
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE:
            fd = lws_get_socket_fd( wsi );
			while( !self->connections[fd]->buffer.empty( ) )
			{
				char *out = NULL;
				int len = strlen(self->connections[fd]->buffer.front( ).c_str());
				out = (char *)malloc(sizeof(char)*(LWS_SEND_BUFFER_PRE_PADDING + len + LWS_SEND_BUFFER_POST_PADDING));
				memcpy (out + LWS_SEND_BUFFER_PRE_PADDING, self->connections[fd]->buffer.front( ).c_str(), len );
				int charsSent = lws_write(wsi, (unsigned char *)out + LWS_SEND_BUFFER_PRE_PADDING, len, LWS_WRITE_TEXT);

				free(out);

				if( charsSent < len )
					self->onErrorWrapper( fd, string( "Error writing to socket" ) );
				else
					// Only pop the message if it was sent successfully.
					self->connections[fd]->buffer.pop_front( );
			}
			if (self->connections[lws_get_socket_fd( wsi )]->Quit == true)
				return -1;
            lws_callback_on_writable( wsi );
            break;

        case LWS_CALLBACK_RECEIVE:
            self->onMessage( lws_get_socket_fd( wsi ), string( (const char *)in ) );
            break;

        case LWS_CALLBACK_CLOSED:
            self->onDisconnectWrapper( lws_get_socket_fd( wsi ) );
            break;

        default:
            break;
    }
    return 0;
}

static struct lws_protocols protocols[] = {
    {
        "/",
        callback_main,
        0, // user data struct not used
        MAX_BUFFER_SIZE,
    },{ NULL, NULL, 0, 0 } // terminator
};

WebSocketServer::WebSocketServer( std::string ip, std::string port, const string certPath, const string& keyPath )
{
    this->_port     = std::stoi(port);
    this->_certPath = certPath;
    this->_keyPath  = keyPath;

    lws_set_log_level( 0, lwsl_emit_syslog ); // We'll do our own logging, thank you.
    struct lws_context_creation_info info;
    memset( &info, 0, sizeof info );
    info.port = this->_port;
    info.iface = ip.c_str();
    info.protocols = protocols;
    info.extensions = NULL;

    if( !this->_certPath.empty( ) && !this->_keyPath.empty( ) )
    {
        info.ssl_cert_filepath        = this->_certPath.c_str( );
        info.ssl_private_key_filepath = this->_keyPath.c_str( );
    }
    else
    {
        info.ssl_cert_filepath        = NULL;
        info.ssl_private_key_filepath = NULL;
    }
    info.gid = -1;
    info.uid = -1;
    info.options = 0;

    // keep alive
    info.ka_time = 20; // 60 seconds until connection is suspicious
    info.ka_probes = 10; // 10 probes after ^ time
    info.ka_interval = 10; // 10s interval for sending probes
    this->_context = lws_create_context( &info );
    if( !this->_context )
        throw "libwebsocket init failed";

    // Some of the libwebsocket stuff is define statically outside the class. This
    // allows us to call instance variables from the outside.  Unfortunately this
    // means some attributes must be public that otherwise would be private.
    self = this;
}

WebSocketServer::~WebSocketServer( )
{
    // Free up some memory
    for( map<int,Connection*>::const_iterator it = this->connections.begin( ); it != this->connections.end( ); ++it )
    {
        Connection* c = it->second;
        this->connections.erase( it->first );
        delete c;
    }
}

void WebSocketServer::onConnectWrapper( int socketID )
{
    Connection* c = new Connection;
    c->createTime = time( 0 );
    this->connections[ socketID ] = c;
    this->onConnect( socketID );
}

void WebSocketServer::onDisconnectWrapper( int socketID )
{
    this->onDisconnect( socketID );
    this->_removeConnection( socketID );
}

void WebSocketServer::onErrorWrapper( int socketID, const string& message )
{
    this->onError( socketID, message );
    this->_removeConnection( socketID );
}

void WebSocketServer::send( int socketID, string data )
{
    // Push this onto the buffer. It will be written out when the socket is writable.
    if (this->connections[socketID])
		this->connections[socketID]->buffer.push_back( std::move(data) );
}

void WebSocketServer::broadcast(string data )
{
    for( map<int,Connection*>::const_iterator it = this->connections.begin( ); it != this->connections.end( ); ++it )
        this->send( it->first, data );
}

void WebSocketServer::setValue( int socketID, const string name, const string value )
{
    this->connections[socketID]->keyValueMap[name] = value;
}

string WebSocketServer::getValue( int socketID, const string name )
{
    return this->connections[socketID]->keyValueMap[name];
}
int WebSocketServer::getNumberOfConnections( )
{
    return this->connections.size( );
}

void WebSocketServer::run( uint64_t timeout )
{
    while( 1 )
    {
        this->wait( timeout );
    }
}

void WebSocketServer::wait( uint64_t timeout )
{
    if( lws_service( this->_context, timeout ) < 0 )
        throw "Error polling for socket activity.";
}

void WebSocketServer::_removeConnection( int socketID )
{
    Connection* c = this->connections[ socketID ];
    this->connections.erase( socketID );
    delete c;
}

void WebSocketServer::Quit( int socketID )
{
	if (this->connections[ socketID ])
		this->connections[ socketID ]->Quit = true;
}

std::string WebSocketServer::IP (int socketID) {
	struct sockaddr_in clientaddr;
	socklen_t addrlen=sizeof(clientaddr);
	struct sockaddr_in6 clientaddr6;
	socklen_t addrlen6=sizeof(clientaddr6);
	char str[INET6_ADDRSTRLEN];
	if (getpeername(socketID, (struct sockaddr *)&clientaddr, &addrlen) == 0) {
		return std::string(inet_ntoa(((struct sockaddr_in*)&clientaddr)->sin_addr));
	} else if (getpeername(socketID, (struct sockaddr *)&clientaddr6, &addrlen6) == 0) {
		inet_ntop(AF_INET6, &(clientaddr6.sin6_addr), str, INET6_ADDRSTRLEN);
		return std::string(str);
	} else return "0.0.0.0";
}
