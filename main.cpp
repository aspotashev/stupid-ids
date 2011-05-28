
#include <stdio.h>
#include <string.h>

#include <gettextpo-helper/detectorbase.h>

#include "tcpcommandserver.h"

class Server : public TcpCommandServer
{
public:
	virtual void commandHandler(const char *command);

private:
	static const char *getCommandArg(const char *input, const char *command);

	void handleGetMinIds(const char *tp_hash_str);
};

const char *Server::getCommandArg(const char *input, const char *command)
{
	size_t cmd_len = strlen(command);
	if (!strncmp(input, command, cmd_len) && input[cmd_len] == ' ')
		return input + cmd_len + 1;
	else
		return NULL;
}

void Server::handleGetMinIds(const char *tp_hash_str)
{
	// "tp_hash" is not the same as "oid", but we can still use
	// the class GitOid for tp_hashes.
	GitOid tp_hash(tp_hash_str);
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
		handleGetMinIds(arg);
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

