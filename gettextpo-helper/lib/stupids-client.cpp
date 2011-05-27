
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <vector>

// for working with stupids-server
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <gettextpo-helper/stupids-client.h>

//-------- Working with stupids-server.rb over TCP/IP --------

const char *TpHashNotFoundException::what() const throw()
{
	return "tp_hash was not found in database";
}

//------------------------------------------------------------

StupidsClient::StupidsClient()
{
	m_sockfd = -1;
}

StupidsClient::~StupidsClient()
{
}

int StupidsClient::sockReadOutput(char **buffer, int *res_bytes)
{
	// Read the size of output in bytes
	char res_bytes_str[10];
	ssize_t res;

	res = read(m_sockfd, res_bytes_str, 10);
	if (res != 10)
	{
		printf("res = %d\n", (int)res);
		assert(0);
	}

	*res_bytes = atoi(res_bytes_str);

	*buffer = new char [*res_bytes + 1];

	char *buffer_cur = *buffer;
	int bytes_cur = *res_bytes;
	while (bytes_cur > 0)
	{
		res = read(m_sockfd, buffer_cur, bytes_cur);
		assert(res > 0);

		bytes_cur -= res;
		buffer_cur += res;
	}

	(*buffer)[*res_bytes] = '\0';

	return 0; // OK
}

void StupidsClient::connect()
{
	if (m_sockfd >= 0)
		return;

	// initialize connection
	struct sockaddr_in servaddr;

	// SOCK_NONBLOCK requires Linux 2.6.27
	if ((m_sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		assert(0);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(1234);
	inet_aton("127.0.0.1", &servaddr.sin_addr);

	if (::connect(m_sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0)
	{
		close(m_sockfd);

		printf("Could not connect to stupids-server\n");
		assert(0);
	};
}

void StupidsClient::disconnect()
{
	const char exit_cmd[] = "exit\n";
	assert(write(m_sockfd, exit_cmd, strlen(exit_cmd)) == strlen(exit_cmd));
	close(m_sockfd);
	m_sockfd = -1;
}

std::vector<int> StupidsClient::getMinIds(const char *tp_hash)
{
	connect();

	// send command
	const char get_min_ids_cmd[] = "get_min_id_array ";
	const char newline[] = "\n";
	assert(strlen(tp_hash) == 40); // sha-1 is 20 bytes long

	assert(write(m_sockfd, get_min_ids_cmd, strlen(get_min_ids_cmd)) == strlen(get_min_ids_cmd));
	assert(write(m_sockfd, tp_hash, 40) == 40);
	assert(write(m_sockfd, newline, strlen(newline)) == strlen(newline));

	// read results

	char *output = NULL;
	int output_len = 0;
	assert(sockReadOutput(&output, &output_len) == 0);

	if (output_len >= 9 && !memcmp(output, "NOTFOUND\n", 9))
	{
		printf("tp_hash not found (%s)\n", tp_hash);
		throw TpHashNotFoundException();
	}

	int id_count = atoi(output);
	char *output_ptr = output;

	std::vector<int> res; // TODO: reserve memory for 'id_count' elements
	for (int i = 0; i < id_count; i ++)
	{
		output_ptr = strchr(output_ptr, ' ');
		assert(output_ptr);
		output_ptr ++; // move from space to the number

		res.push_back(atoi(output_ptr));
	}

//	disconnect();

	return res;
}

