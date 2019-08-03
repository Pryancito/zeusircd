#include "ZeusBaseClass.h"
#include <thread>

int main (int argc, char *argv[])
{
	PublicSock::Listen("127.0.0.1", "6667");
	
	return 0;
}
