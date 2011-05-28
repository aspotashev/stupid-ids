/* Sample TCP server */

#include <stdio.h>
#include <strings.h>
#include <assert.h>

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

	void sessionOpened();

private:
	int m_connfd;
	struct sockaddr_in m_cliaddr;
	socklen_t m_clilen;
};

Server::Server()
{
}

Server::~Server()
{
}

void Server::sessionOpened()
{
	const int BUFFER_SZ = 4096;
	char buffer[BUFFER_SZ];
	int buffer_len = 0;

	for (;;)
	{
		assert(BUFFER_SZ > buffer_len); // check that the buffer is not full

		int read_bytes = recvfrom(m_connfd, buffer + buffer_len, BUFFER_SZ - buffer_len,
			0, (struct sockaddr *)&m_cliaddr, &m_clilen);
		sendto(m_connfd, buffer, read_bytes,
			0, (struct sockaddr *)&m_cliaddr, sizeof(m_cliaddr));

//		const char *newline = strchr(mesg, )

//		printf("-------------------------------------------------------\n");
//		printf("Received the following:\n");
//		printf("%s",mesg);
//		printf("-------------------------------------------------------\n");
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
	bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	listen(listenfd, 1024);

	printf("Server started.\n");

	for (;;)
	{
		m_clilen = sizeof(sockaddr_in);
		m_connfd = accept(listenfd, (struct sockaddr *)&m_cliaddr, &m_clilen);

		if ((childpid = fork()) == 0)
		{
			close(listenfd);

			sessionOpened();
		}
		close(m_connfd);
	}
}

int main()
{
	Server server;
	server.start();

	return 0;
}

