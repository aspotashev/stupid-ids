
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include <unistd.h>
#include <sys/wait.h>

#include "tcpcommandserver.h"

TcpCommandServer::TcpCommandServer()
{
	m_closeConnection = false;
}

TcpCommandServer::~TcpCommandServer()
{
}

void TcpCommandServer::sendToClient(const void *data, size_t len)
{
	sendto(m_connfd, data, len,
		0, (struct sockaddr *)&m_cliaddr, sizeof(m_cliaddr));
}

int TcpCommandServer::recvFromClient(void *data, size_t len)
{
	ssize_t read_bytes = recvfrom(m_connfd, data, len,
		0, (struct sockaddr *)&m_cliaddr, &m_clilen);
	if (read_bytes == 0)
	{
		printf("Client has disconnected.\n");
		return -1;
	}
	else if (read_bytes < 0)
	{
		printf("Failed to read from socket.\n");
		return -2;
	}

	assert(read_bytes == (ssize_t)len);

	return 0;
}

void TcpCommandServer::sessionOpened()
{
	uint32_t command;

	while (!m_closeConnection)
	{
		if (recvFromClient(&command, 4) != 0)
			break;

		commandHandler(ntohl(command));
	}
}

// SIGCHLD signal handler
void sigchild_handler (int signum)
{
	int status;
	int pid;

	while (1)
	{
		// Killing zombies
		pid = waitpid (-1, &status, WNOHANG);
		if (pid > 0)
		{
			printf ("A child process has terminated (PID = %d, status = %d)\n", pid, status);
			assert(status == 0);
		}
		else if (pid == 0 || (pid < 0 && errno == ECHILD))
		{
//			printf ("Somebody is joking with SIGCHLD...\n");
			break;
		}
		else
		{
			printf ("waitpid failed\n");
			break;
		}
	}
}

// TODO: This needs a complete rewrite.
// I will never copy-paste educational source code again.
// I will never copy-paste educational source code again.
// I will never copy-paste educational source code again.
// I will never copy-paste educational source code again.
// I will never copy-paste educational source code again.
//
// TODO: kill zombies
void TcpCommandServer::start()
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

	assert(signal (SIGCHLD, sigchild_handler) != SIG_ERR);

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
			exit(0);
		}
		close(m_connfd);

		assert(childpid > 0); // check that fork() was successful
	}
}

void TcpCommandServer::disconnect()
{
	m_closeConnection = true;
}

