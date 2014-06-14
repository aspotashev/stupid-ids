
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
	m_outputBuffer.append((const char *)data, len);
}

void TcpCommandServer::writeToClient(const void *data, size_t len)
{
	assert (!m_closeConnection);

    ssize_t ret = sendto(
        m_connfd, data, len,
        0, (struct sockaddr *)&m_cliaddr, sizeof(m_cliaddr));
    assert(ret == static_cast<ssize_t>(len));
}

void TcpCommandServer::flushToClient()
{
	// Send error code: 0 = no error
	uint32_t code = htonl(0); // pedantic :)
	writeToClient(&code, sizeof(code));

	writeToClient(m_outputBuffer.data(), m_outputBuffer.size());
	m_outputBuffer.clear();
}

void TcpCommandServer::flushErrorToClient(uint32_t code)
{
	code = htonl(code);
	writeToClient(&code, sizeof(code));
	m_outputBuffer.clear();
}

void TcpCommandServer::recvFromClient(void *data, size_t len)
{
	if (m_closeConnection)
		throw ExceptionNoConnection();

	size_t bytes_read = 0;

	while (bytes_read < len)
	{
		ssize_t res = recvfrom(m_connfd, (char *)data + bytes_read, len - bytes_read,
			0, (struct sockaddr *)&m_cliaddr, &m_clilen);
		if (res == 0)
		{
			printf("Client has disconnected.\n");
			disconnect();
			throw ExceptionDisconnected();
		}
		else if (res < 0)
		{
			printf("Failed to read from socket.\n");
			disconnect();
			throw ExceptionRecvFailed();
		}

		bytes_read += res;
	}
}

void TcpCommandServer::sessionOpened()
{
	while (!m_closeConnection)
		commandHandler();
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
			int code = WEXITSTATUS(status); // argument passed to exit() in the child process
			printf ("A child process has terminated (PID = %d, status = %d, code = %d)\n", pid, status, code);
			assert(code == 0 || code == 1); // 1 means that excetion has been raised
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

//--------------------------------

const char *TcpCommandServer::ExceptionNoConnection::what() const throw()
{
	return "ExceptionNoConnection (connection has already been closed)";
}

const char *TcpCommandServer::ExceptionDisconnected::what() const throw()
{
	return "ExceptionDisconnected (connection has just been closed)";
}

const char *TcpCommandServer::ExceptionRecvFailed::what() const throw()
{
	return "ExceptionRecvFailed (failed to read from socket)";
}

