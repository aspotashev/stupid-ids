
#include <stdio.h>
#include <assert.h>

#include <gettextpo-helper/stupids-client.h>
#include <gettextpo-helper/mappedfile.h>

#include "server.h"
#include "filedbfirstids.h"

Server::Server()
{
	m_firstIds = new FiledbFirstIds(
		"../stupid-id-filler/ids/first_ids.txt",
		"../stupid-id-filler/ids/next_id.txt");
	m_idMapDb = new MappedFileIdMapDb("../transition-detector/idmap.mmapdb");
}

Server::~Server()
{
	delete m_firstIds;
	delete m_idMapDb;
}

GitOid Server::recvOid()
{
	static unsigned char oid_raw[GIT_OID_RAWSZ];
	assert(recvFromClient(oid_raw, GIT_OID_RAWSZ) == 0);
	GitOid oid;
	oid.setOidRaw(oid_raw);

	return oid;
}

void Server::handleGetMinIdArray()
{
	// "tp_hash" is not the same as "oid", but we can still use
	// the class GitOid for tp_hashes.
	GitOid tp_hash = recvOid();

	std::pair<int, int> first_ids = m_firstIds->getFirstId(tp_hash);
	int first_id = first_ids.first;
	int id_count = first_ids.second;
	assert(first_id != 0);

	uint32_t *output = new uint32_t[id_count + 1];
	output[0] = htonl((uint32_t)id_count);
	for (int i = 0; i < id_count; i ++)
	{
		int min_id = m_idMapDb->getPlainMinId(first_id + i);
		output[i + 1] = htonl((uint32_t)min_id);
	}

	sendToClient(output, sizeof(uint32_t) * (id_count + 1));

	delete [] output;
}

std::vector<int> Server::recvLongVector()
{
	uint32_t count = recvLong();

	uint32_t *arr = new uint32_t[(size_t)count];
	recvFromClient(arr, count * 4);

	std::vector<int> res;
	for (size_t i = 0; i < count; i ++)
		res.push_back((int)ntohl(arr[i]));
	delete [] arr;

	return res;
}

void Server::handleGetMinIds()
{
	std::vector<int> ids_arr = recvLongVector();

	for (size_t i = 0; i < ids_arr.size(); i ++)
		ids_arr[i] = m_idMapDb->getPlainMinId(ids_arr[i]);


	uint32_t *ids_arr_raw = new uint32_t[(size_t)ids_arr.size()];
	for (size_t i = 0; i < ids_arr.size(); i ++)
		ids_arr_raw[i] = htonl((uint32_t)ids_arr[i]);

	sendToClient(ids_arr_raw, ids_arr.size() * 4);
	delete [] ids_arr_raw;
}

void Server::sendLong(uint32_t data)
{
	data = htonl(data);
	sendToClient(&data, sizeof(uint32_t));
}

void Server::handleGetFirstId()
{
	// "tp_hash" is not the same as "oid", but we can still use
	// the class GitOid for tp_hashes.
	GitOid tp_hash = recvOid();

	std::pair<int, int> first_ids = m_firstIds->getFirstId(tp_hash);
	int first_id = first_ids.first;
	assert(first_id != 0);

	sendLong((uint32_t)first_id);
}

uint32_t Server::recvLong()
{
	uint32_t res;
	assert (recvFromClient(&res, 4) == 0);
	return ntohl(res);
}

void Server::commandHandler()
{
	uint32_t command = recvLong();

	if (command == StupidsClient::CMD_EXIT)
		disconnect();
	else if (command == StupidsClient::CMD_GET_MIN_ID_ARRAY)
		handleGetMinIdArray();
	else if (command == StupidsClient::CMD_GET_FIRST_ID)
		handleGetFirstId();
	else if (command == StupidsClient::CMD_GET_MIN_IDS)
		handleGetMinIds();
	else
	{
		printf("Unknown command code %d.\n", command);
		disconnect();
		assert(0);
	}
}

