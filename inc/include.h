#ifndef INCLUDE_H
#define INCLUDE_H

#define MAX_USERS 65000
#define BOOST_ASIO_DISABLE_HANDLER_TYPE_REQUIREMENTS

#include <ulimit.h>
#include <iostream>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
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
#include <sstream>
#include <deque>

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread.hpp>
#include <boost/spirit/include/qi_char_class.hpp>

#include "clases.h"
#include "extern.h"

#endif
