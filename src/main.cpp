#include "ZeusBaseClass.h"
#include <thread>

int main (int argc, char *argv[])
{
	PublicSock::Listen("192.168.0.102", "6667");
	
	return 0;
}
