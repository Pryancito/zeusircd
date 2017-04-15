#ifndef INCLUDE_H
#define INCLUDE_H

#define MAX_USERS 1024
<<<<<<< HEAD
#define SENDQ 2000
=======
#define SENDQ 3000000
>>>>>>> 04d2c448a030e2e425470ace29566c76d26b6751
#define _XOPEN_SOURCE

#include <ulimit.h>
#include <iostream>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <thread>
#include <ctype.h>
#include <cctype>
#include <csignal>
#include <fcntl.h>
#include <mutex>
#include <random>
#include <sys/stat.h>
#include <errno.h>
#include <syslog.h>
#include <fstream>
#include <sqlite3.h>
#include <ctime>

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "extern.h"
#include "clases.h"
#include "tcp.h"

#endif
