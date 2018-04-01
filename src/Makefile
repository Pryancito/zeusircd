# Project: ZeusiRCd

CPP      	= g++
CC       	= gcc
OBJS	= main.o \
				mainframe.o \
				channel.o \
				config.o \
				server.o \
				user.o \
				session.o \
				parser.o \
				parserserver.o \
				oper.o \
				sha256.o \
				utils.o \
				db.o \
				nickserv.o \
				chanserv.o \
				hostserv.o \
				operserv.o \
				api.o
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
