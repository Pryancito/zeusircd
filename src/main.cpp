#include "ZeusBaseClass.h"
#include <thread>

int main (int argc, char *argv[])
{
	PublicSock::Listen("::", "6667");
	
	return 0;
}
