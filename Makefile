# Project: Zeus
# Makefile created by Dev-C++ 5.11

CPP      = g++
CC       = gcc
OBJ      = main.o src/lista.o src/nodes.o src/socket.o src/user.o src/chan.o src/config.o src/oper.o src/server.o src/db.o src/nickserv.o src/chanserv.o src/sha256.o
LINKOBJ  = main.o src/lista.o src/nodes.o src/socket.o src/user.o src/chan.o src/config.o src/oper.o src/server.o src/db.o src/nickserv.o src/chanserv.o src/sha256.o
LIBS     = -L"./lib" -static-libgcc -g3 -L"/usr/lib/" -L"/usr/local/lib" -lboost_system -pthread -lsqlite3 -lssl -lcrypto -lboost_thread
INCS     = -I"./inc" -I"/usr/include/" -I"/usr/local/include"
CXXINCS  = -I"./inc" -I"/usr/include/" -I"/usr/local/include"
BIN      = Zeus
CXXFLAGS = $(CXXINCS) -g3 -std=c++11 -Wall
CFLAGS   = $(INCS) -g3 -std=c++11 -Wall
RM       = rm -f

.PHONY: all all-before all-after clean clean-custom

all: all-before $(BIN) all-after

clean: clean-custom
	${RM} $(OBJ) $(BIN)

$(BIN): $(OBJ)
	$(CPP) $(LINKOBJ) -o $(BIN) $(LIBS)

main.o: main.cpp
	$(CPP) -c main.cpp -o main.o $(CXXFLAGS)

src/nodes.o: src/nodes.cpp
	$(CPP) -c src/nodes.cpp -o src/nodes.o $(CXXFLAGS)

src/lista.o: src/lista.cpp
	$(CPP) -c src/lista.cpp -o src/lista.o $(CXXFLAGS)

src/socket.o: src/socket.cpp
	$(CPP) -c src/socket.cpp -o src/socket.o $(CXXFLAGS)

src/user.o: src/user.cpp
	$(CPP) -c src/user.cpp -o src/user.o $(CXXFLAGS)

src/chan.o: src/chan.cpp
	$(CPP) -c src/chan.cpp -o src/chan.o $(CXXFLAGS)

src/config.o: src/config.cpp
	$(CPP) -c src/config.cpp -o src/config.o $(CXXFLAGS)

src/oper.o: src/oper.cpp
	$(CPP) -c src/oper.cpp -o src/oper.o $(CXXFLAGS)

src/server.o: src/server.cpp
	$(CPP) -c src/server.cpp -o src/server.o $(CXXFLAGS)

src/db.o: src/db.cpp
	$(CPP) -c src/db.cpp -o src/db.o $(CXXFLAGS)

src/nickserv.o: src/nickserv.cpp
	$(CPP) -c src/nickserv.cpp -o src/nickserv.o $(CXXFLAGS)
	
src/chanserv.o: src/chanserv.cpp
	$(CPP) -c src/chanserv.cpp -o src/chanserv.o $(CXXFLAGS)
	
src/sha256.o: src/sha256.cpp
	$(CPP) -c src/sha256.cpp -o src/sha256.o $(CXXFLAGS)

