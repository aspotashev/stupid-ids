
#include <string.h>

#include "tcpcommandserver.h"

class Server : public TcpCommandServer
{
public:
	virtual void commandHandler(const char *command);
};

void Server::commandHandler(const char *command)
{
	sendToClient(command, strlen(command));
	sendToClient("\n", 1);
}

//-------------------

int main()
{
	Server server;
	server.start();

	return 0;
}

