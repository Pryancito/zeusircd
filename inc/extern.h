#ifndef EXTERN_H
#define EXTERN_H

#include "tcp.h"
#include "clases.h"
#include <sqlite3.h>
#include <mutex>
#include <queue>

struct infocola {
	TCPStream *stream;
	string mensaje;
};

struct ban {
	string mascara;
	string who;
	unsigned long fecha;
};

struct user {
	string nombre;
	char modo;
};
	
extern Config *config;
extern Oper *oper;
extern Socket *sock;
extern Server *server;
extern Cliente *cliente;
extern Chan *chan;
extern Nick *nick;
extern Data *datos;
extern NickServ *nickserv;
extern DB *db;
extern ChanServ *chanserv;

string mayus (const string str);
void mayuscula (string &str);

extern mutex nick_mute, chan_mute, server_mute, oper_mute, memo_mute;
void procesacola ();

extern Semaforo semaforo;
extern std::queue <infocola> cola;
extern time_t encendido;
/* Databases */

#endif
