#include "ZeusBaseClass.h"
#include <thread>

int main (int argc, char *argv[])
{
	PublicSock::SSListen("127.0.0.1", "6697");
	
	return 0;
}
