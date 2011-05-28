
#include <stdio.h>
#include <string.h>

#include "tcpcommandserver.h"

class Server : public TcpCommandServer
{
public:
	virtual void commandHandler(const char *command);

private:
	static const char *getCommandArg(const char *input, const char *command);
};

const char *Server::getCommandArg(const char *input, const char *command)
{
	size_t cmd_len = strlen(command);
	if (!strncmp(input, command, cmd_len) && input[cmd_len] == ' ')
		return input + cmd_len + 1;
	else
		return NULL;
}

void Server::commandHandler(const char *command)
{
	const char *arg;

	if (!strcmp(command, "exit"))
	{
		disconnect();
	}
	else if (arg = getCommandArg(command, "get_min_id_array"))
	{
		printf("tp_hash = %s\n", arg);
	}
	else
	{
		sendToClient(command, strlen(command));
		sendToClient("\n", 1);
	}
}

//-------------------

int main()
{
	Server server;
	server.start();

	return 0;
}

