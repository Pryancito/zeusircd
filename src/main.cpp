#include "ZeusBaseClass.h"
#include <thread>
#include <set>
#include <iostream>
#include <ulimit.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <ulimit.h>
#include <sys/resource.h>
#include <csignal>
#include <sys/time.h>

extern std::map<std::string, unsigned int> mThrottle;
std::vector<std::thread*> Threads;
bool exiting = false;

void write_pid () {
	ofstream procid("zeus.pid");
	procid << getpid() << endl;
	procid.close();
}
void doexit() {
	if (!exiting) {
		//Servidor::sendall("SQUIT " + config->Getvalue("serverName"));
		system("rm -f zeus.pid");
		for (unsigned int i = 0; i < Threads.size(); i++)
			delete Threads[i];
		exiting = true;
		std::cout << "Exiting Zeus." << std::endl;
	}
	exit(0);
}

void sHandler( int signum ) {
	doexit();
}

int main (int argc, char *argv[])
{
	write_pid();
	atexit(doexit);
	signal(SIGINT, sHandler);
	signal(SIGTERM, sHandler);

	std::thread *t = new std::thread(&PublicSock::Listen, "127.0.0.1", "6667");
	Threads.push_back(t);
	t->detach();
	
	while (!exiting) {
		sleep(30);
		mThrottle.clear();
	}
	return 0;
}
