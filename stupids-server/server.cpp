
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

//-----------------------

uint32_t Server::recvLong()
{
	uint32_t res;
	assert (recvFromClient(&res, 4) == 0);
	return ntohl(res);
}

GitOid Server::recvOid()
{
	static unsigned char oid_raw[GIT_OID_RAWSZ];
	assert(recvFromClient(oid_raw, GIT_OID_RAWSZ) == 0);
	GitOid oid;
	oid.setOidRaw(oid_raw);

	return oid;
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

//-----------------------

void Server::sendLong(uint32_t data)
{
	data = htonl(data);
	sendToClient(&data, sizeof(uint32_t));
}

void Server::sendLongArray(std::vector<int> arr)
{
	uint32_t *arr_raw = new uint32_t[(size_t)arr.size()];
	for (size_t i = 0; i < arr.size(); i ++)
		arr_raw[i] = htonl((uint32_t)arr[i]);

	sendToClient(arr_raw, sizeof(uint32_t) * arr.size());
	delete [] arr_raw;
}

void Server::sendLongVector(std::vector<int> vec)
{
	sendLong((uint32_t)vec.size());
	sendLongArray(vec);
}

//-----------------------

void Server::handleGetMinIdArray()
{
	// "tp_hash" is not the same as "oid", but we can still use
	// the class GitOid for tp_hashes.
	GitOid tp_hash = recvOid();

	std::pair<int, int> first_ids = m_firstIds->getFirstId(tp_hash);
	int first_id = first_ids.first;
	int id_count = first_ids.second;
	assert(first_id != 0);

	std::vector<int> output(id_count);
	for (int i = 0; i < id_count; i ++)
		output[i] = m_idMapDb->getPlainMinId(first_id + i);
	sendLongVector(output);
}

void Server::handleGetMinIds()
{
	std::vector<int> ids = recvLongVector();

	for (size_t i = 0; i < ids.size(); i ++)
		ids[i] = m_idMapDb->getPlainMinId(ids[i]);

	sendLongArray(ids);
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

