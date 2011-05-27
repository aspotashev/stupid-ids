
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
}

StupidsClient::~StupidsClient()
{
}

int StupidsClient::sockReadOutput(int sockfd, char **buffer, int *res_bytes)
{
	// Read the size of output in bytes
	char res_bytes_str[10];
	ssize_t res;

	res = read(sockfd, res_bytes_str, 10);
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
		res = read(sockfd, buffer_cur, bytes_cur);
		assert(res > 0);

		bytes_cur -= res;
		buffer_cur += res;
	}

	(*buffer)[*res_bytes] = '\0';

	return 0; // OK
}

// TODO: use a single connection for all requests
std::vector<int> StupidsClient::getMinIds(const char *tp_hash)
{
	// initialize connection
	struct sockaddr_in servaddr;
	int sockfd;

	// SOCK_NONBLOCK requires Linux 2.6.27
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		assert(0);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(1234);
	inet_aton("127.0.0.1", &servaddr.sin_addr);

	if (connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0)
	{
		close(sockfd);

		printf("Could not connect to stupids-server\n");
		assert(0);
	};

	// send command
	const char get_min_ids_cmd[] = "get_min_id_array ";
	const char newline[] = "\n";
	assert(strlen(tp_hash) == 40); // sha-1 is 20 bytes long

	assert(write(sockfd, get_min_ids_cmd, strlen(get_min_ids_cmd)) == strlen(get_min_ids_cmd));
	assert(write(sockfd, tp_hash, 40) == 40);
	assert(write(sockfd, newline, strlen(newline)) == strlen(newline));

	// read results

	char *output = NULL;
	int output_len = 0;
	assert(sockReadOutput(sockfd, &output, &output_len) == 0);

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

	const char exit_cmd[] = "exit\n";
	assert(write(sockfd, exit_cmd, strlen(exit_cmd)) == strlen(exit_cmd));
	close(sockfd);

	return res;
}

