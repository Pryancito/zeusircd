#include "ZeusBaseClass.h"
#include <thread>

int main (int argc, char *argv[])
{
	PublicSock::WebListen("127.0.0.1", "8000");
	
	return 0;
}
