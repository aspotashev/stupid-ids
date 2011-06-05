
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <vector>

// for working with stupids-server
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#include <gettextpo-helper/stupids-client.h>

//-------- Working with stupids-server.rb over TCP/IP --------

StupidsClient *StupidsClient::s_instance = NULL;

StupidsClient::StupidsClient()
{
	m_sockfd = -1;
}

StupidsClient::~StupidsClient()
{
}

void StupidsClient::recvFromServer(void *data, size_t len)
{
	int bytes_read = 0;

	while (bytes_read < len)
	{
		int res = read(m_sockfd, (char *)data + bytes_read, len - bytes_read);
		assert(res > 0);

		bytes_read += res;
	}
}

uint32_t StupidsClient::recvLong()
{
	uint32_t res = 0xffffffff;
	recvFromServer(&res, sizeof(uint32_t));
	return ntohl(res);
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

void StupidsClient::sendLong(uint32_t data)
{
	data = htonl(data);
	sendToServer(&data, 4);
}

void StupidsClient::sendOid(const git_oid *oid)
{
	sendToServer(oid, GIT_OID_RAWSZ);
}

void StupidsClient::sendToServer(const void *data, size_t len)
{
	size_t written = 0;
	while (written < len)
	{
		ssize_t res = write(m_sockfd, data, len);
		if (res <= 0)
		{
			printf("res = %d, len = %d, errno = %d\n", (int)res, (int)len, errno);
			assert(0);
		}

		written += res;
	}

	assert(written == len);
}

void StupidsClient::disconnect()
{
	sendLong(CMD_EXIT);

	close(m_sockfd);
	m_sockfd = -1;
}

std::vector<int> StupidsClient::getMinIds(const git_oid *tp_hash)
{
	connect();

	// send command
	sendLong(CMD_GET_MIN_ID_ARRAY);
	sendOid(tp_hash);

	// read results
	int id_count = (int)recvLong();
	assert(id_count >= 0);

	uint32_t *first_ids = new uint32_t[id_count];
	recvFromServer(first_ids, sizeof(uint32_t) * id_count);

	std::vector<int> res; // TODO: reserve memory for 'id_count' elements
	for (int i = 0; i < id_count; i ++)
		res.push_back((int)ntohl(first_ids[i]));


//	if (output_len >= 9 && !memcmp(output, "NOTFOUND\n", 9))
//	{
//		printf("tp_hash not found (%s)\n", tp_hash);
//		throw TpHashNotFoundException();
//	}


	return res;
}

std::vector<int> StupidsClient::getMinIds(std::vector<int> msg_ids)
{
	connect();

	size_t id_count = msg_ids.size();
	uint32_t *ids_arr = new uint32_t[id_count];

	// send command
	sendLong(CMD_GET_MIN_IDS);
	sendLong((uint32_t)id_count);
	for (size_t i = 0; i < id_count; i ++)
		ids_arr[i] = htonl((uint32_t)msg_ids[i]);
	sendToServer(ids_arr, id_count * 4);

	// read results
	recvFromServer(ids_arr, id_count * 4);

	std::vector<int> res;
	for (size_t i = 0; i < id_count; i ++)
		res.push_back((int)ntohl(ids_arr[i]));
	delete [] ids_arr;

	return res;
}

int StupidsClient::getFirstId(const git_oid *tp_hash)
{
	connect();

	// send command
	sendLong(CMD_GET_FIRST_ID);
	sendOid(tp_hash);

	// read results
	int first_id = (int)recvLong();

	assert(first_id > 0);
	return first_id;
}

StupidsClient &StupidsClient::instance()
{
	if (s_instance == NULL)
	{
		s_instance = new StupidsClient();
	}

	return *s_instance;
}

