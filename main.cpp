/* Sample TCP server */

#include <stdio.h>
#include <strings.h>
#include <assert.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

void sessionOpened(int connfd, struct sockaddr *cliaddr, socklen_t *clilen)
{
	const int BUFFER_SZ = 4096;
	char buffer[BUFFER_SZ];
	int buffer_len = 0;

	for (;;)
	{
		assert(BUFFER_SZ > buffer_len); // check that the buffer is not full

		int read_bytes = recvfrom(connfd, buffer + buffer_len, BUFFER_SZ - buffer_len, 0, cliaddr, clilen);
		sendto(connfd, buffer, read_bytes, 0, cliaddr, sizeof(sockaddr_in));

//		const char *newline = strchr(mesg, )

//		printf("-------------------------------------------------------\n");
//		printf("Received the following:\n");
//		printf("%s",mesg);
//		printf("-------------------------------------------------------\n");
	}
}

int main(int argc, char**argv)
{
	int listenfd, connfd;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t clilen;
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
		clilen = sizeof(sockaddr_in);
		connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);

		if ((childpid = fork()) == 0)
		{
			close(listenfd);

			sessionOpened(connfd, (struct sockaddr *)&cliaddr, &clilen);
		}
		close(connfd);
	}

	return 0;
}

