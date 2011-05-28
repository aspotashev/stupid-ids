
// http://www.cs.ucsb.edu/~almeroth/classes/W01.176B/hw2/examples/tcp-server.c

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <errno.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

class Server
{
public:
	Server();
	~Server();

	// Enters an infinite loop
	void start();

	void disconnect();

	void sessionOpened();
	void commandHandler(const char *command);
	void sendToClient(const char *str, size_t len);

private:
	int m_connfd;
	struct sockaddr_in m_cliaddr;
	socklen_t m_clilen;

	bool m_closeConnection;
};

Server::Server()
{
	m_closeConnection = false;
}

Server::~Server()
{
}

void Server::sendToClient(const char *str, size_t len)
{
	sendto(m_connfd, str, len,
		0, (struct sockaddr *)&m_cliaddr, sizeof(m_cliaddr));
}

void Server::commandHandler(const char *command)
{
	sendToClient(command, strlen(command));
	sendToClient("\n", 1);
}

void Server::sessionOpened()
{
	const int BUFFER_SZ = 4096;
	char buffer[BUFFER_SZ];
	int buffer_len = 0;

	while (!m_closeConnection)
	{
		assert(BUFFER_SZ > buffer_len); // check that the buffer is not full

		int read_bytes = recvfrom(m_connfd, buffer + buffer_len, BUFFER_SZ - buffer_len,
			0, (struct sockaddr *)&m_cliaddr, &m_clilen);
		if (read_bytes < 0)
		{
			printf("Client (probably) disconnected unexpectedly.\n");
			break;
		}

		buffer_len += read_bytes;

		char *newline = (char *)memchr(buffer, '\n', buffer_len);
		if (newline)
		{
			*newline = '\0';
			if (newline > buffer && newline[-1] == '\r')
				newline[-1] = '\0';
			commandHandler(buffer);

			// substract the length of the current command (including "\n")
			buffer_len -= newline - buffer + 1;

			// erase the current command
			memmove(buffer, newline + 1, buffer_len);
		}
	}
}

void Server::start()
{
	int listenfd;
	struct sockaddr_in servaddr;
	pid_t childpid;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(1234);
	if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
	{
		printf("bind() failed, errno = %d\n", errno);
		assert(0);
	}

	assert(listen(listenfd, 1024) == 0);

	printf("Server started.\n");

	for (;;)
	{
		m_clilen = sizeof(sockaddr_in);
		m_connfd = accept(listenfd, (struct sockaddr *)&m_cliaddr, &m_clilen);
		assert(m_connfd >= 0);

		if ((childpid = fork()) == 0)
		{
			close(listenfd);

			sessionOpened();
		}
		close(m_connfd);

		assert(childpid > 0); // check that fork() was successful
	}
}

int main()
{
	Server server;
	server.start();

	return 0;
}

