# Project: ZeusiRCd

CPP      	= g++
CC       	= gcc
OBJS      	= main.o \
		src/socket.o \
		src/user.o \
		src/chan.o \
		src/config.o \
		src/oper.o \
		src/server.o \
		src/db.o \
		src/nickserv.o \
		src/chanserv.o \
		src/hostserv.o \
		src/operserv.o \
		src/api.o \
		src/sha256.o
DIRS 		= -L/lib -static-libgcc -g3 -L/usr/lib/ -L/usr/local/lib
LIBS     	= -lboost_system -pthread -lsqlite3 -lssl -lcrypto -lboost_thread -lmicrohttpd
CXXINCS  	= -I./include -I/usr/include/ -I/usr/local/include
BIN      	= Zeus
CXXFLAGS 	= -g3 -std=c++14 -Wall $(CXXINCS)
RM       	= rm -f

.PHONY: all all-before all-after clean clean-custom

all: all-before $(BIN) all-after

clean: clean-custom
	${RM} $(OBJS) $(BIN)

$(BIN): $(OBJS)
	$(CPP) -g -o $(BIN) $(OBJS) $(DIRS) $(LIBS)

%.o: %.cpp
	$(CPP) $(CXXFLAGS) -c $< -o $@
