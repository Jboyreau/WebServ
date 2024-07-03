#include "Server.h"
Server* serverInstance = NULL;

void signalHandler(int signum)
{
	if (serverInstance)
	{
		delete serverInstance;
		serverInstance = NULL;
	}	
	exit(signum);
}

int main(void)
{
	serverInstance = NULL;
	signal(SIGINT, signalHandler);  // gere Ctrl+C.
	signal(SIGTERM, signalHandler); // gere termination request.
	signal(SIGSEGV, signalHandler); // gere segmentation fault.
	
	serverInstance = new Server();
	serverInstance->run();
	delete serverInstance;
	return EXIT_SUCCESS;
}
