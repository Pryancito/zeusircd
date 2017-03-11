#ifndef EXTERN_H
#define EXTERN_H

#include "tcp.h"
#include "clases.h"
#include <sqlite3.h>
#include <mutex>

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

string mayus (const string str);
void mayuscula (string &str);

extern mutex nick_mute, chan_mute, server_mute, oper_mute;
void procesacola ();

extern bool signaled;
extern pthread_mutex_t myMutex;
extern pthread_cond_t cond;
extern time_t encendido;

/* Databases */

#endif
