# Project: ZeusiRCd

CPP      	= g++
CC       	= gcc
OBJS	= main.o \
				socket.o \
				user.o \
				chan.o \
				config.o \
				oper.o \
				server.o \
				db.o \
				nickserv.o \
				chanserv.o \
				hostserv.o \
				operserv.o \
				api.o \
				sha256.o
DIRS 		= -L/lib -static-libgcc -g3 -L/usr/lib/ -L/usr/local/lib
LIBS     	= -lboost_system -pthread -lsqlite3 -lssl -lcrypto -lboost_thread -lmicrohttpd
CXXINCS  	= -I../include -I/usr/include/ -I/usr/local/include
BIN      	= ../Zeus
CXXFLAGS 	= -g3 -std=c++14 -Wall $(CXXINCS)
RM       	= rm -f

.PHONY: all clean

all: info $(BIN)

clean:
	@echo "Cleaning workspace..."
	@${RM} $(OBJS) $(BIN)
	@echo "done."
info:
	@echo "Building ZeusIRCD, this operation may take several minutes..."

$(BIN): $(OBJS)
	@$(CPP) -g -o $(BIN) $(OBJS) $(DIRS) $(LIBS)
	@echo "All done!"

%.o: %.cpp
	@echo "Building $@ from $<, please wait..."
	@$(CPP) $(CXXFLAGS) -c $< -o $@
