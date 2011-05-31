
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

void StupidsClient::sockReadBlock(void *buffer, int length)
{
	int bytes_read = 0;

	while (bytes_read < length)
	{
		int res = read(m_sockfd, (char *)buffer + bytes_read, length - bytes_read);
		assert(res > 0);

		bytes_read += res;
	}
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

void StupidsClient::sendString(const char *str)
{
	size_t len = strlen(str);
	assert(write(m_sockfd, str, len) == len);
}

void StupidsClient::disconnect()
{
	const char exit_cmd[] = "exit\n";
	assert(write(m_sockfd, exit_cmd, strlen(exit_cmd)) == strlen(exit_cmd));
	close(m_sockfd);
	m_sockfd = -1;
}

std::vector<int> StupidsClient::getMinIds(const git_oid *tp_hash)
{
	connect();

	char tp_hash_str[GIT_OID_HEXSZ + 1];
	git_oid_fmt(tp_hash_str, tp_hash);

	// send command
	sendString("get_min_id_array ");
	sendString(tp_hash_str);
	sendString("\n");

	// read results
	uint32_t id_count = -1;
	sockReadBlock(&id_count, sizeof(uint32_t));
	id_count = ntohl(id_count);

	assert((int)id_count >= 0);

	uint32_t *first_ids = new uint32_t[(int)id_count];
	sockReadBlock(first_ids, sizeof(uint32_t) * id_count);

	std::vector<int> res; // TODO: reserve memory for 'id_count' elements
	for (int i = 0; i < (int)id_count; i ++)
		res.push_back((int)ntohl(first_ids[i]));


//	if (output_len >= 9 && !memcmp(output, "NOTFOUND\n", 9))
//	{
//		printf("tp_hash not found (%s)\n", tp_hash);
//		throw TpHashNotFoundException();
//	}


//	disconnect();

	return res;
}

int StupidsClient::getFirstId(const char *tp_hash)
{
	connect();

	assert(strlen(tp_hash) == 40); // sha-1 is 20 bytes long

	// send command
	sendString("get_first_id ");
	sendString(tp_hash);
	sendString("\n");

	// read results
	uint32_t first_id = 0;
	sockReadBlock(&first_id, sizeof(uint32_t));
	int res = (int)ntohl(first_id);

	assert(res > 0);
	return res;
}

