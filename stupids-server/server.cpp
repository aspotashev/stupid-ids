
#include <stdio.h>
#include <assert.h>
#include <algorithm>

#include <gettextpo-helper/stupids-client.h>
#include <gettextpo-helper/mappedfile.h>

#include "server.h"
#include "filedbfirstids.h"

Server::Server()
{
	m_firstIds = new FiledbFirstIds(
		"/home/sasha/stupid-ids/stupid-id-filler/ids/first_ids.txt",
		"/home/sasha/stupid-ids/stupid-id-filler/ids/next_id.txt");
	m_idMapDb = new MappedFileIdMapDb("/home/sasha/stupid-ids/transition-detector/idmap.mmapdb");
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
	recvFromClient(&res, 4);
	return ntohl(res);
}

GitOid Server::recvOid()
{
	static unsigned char oid_raw[GIT_OID_RAWSZ];
	recvFromClient(oid_raw, GIT_OID_RAWSZ);
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

std::vector<int> Server::getTphashMinIds(GitOid tp_hash)
{
	std::pair<int, int> first_ids = m_firstIds->getFirstId(tp_hash);
	int first_id = first_ids.first;
	int id_count = first_ids.second;
	if (first_id == 0)
		throw ExceptionTpHashNotFound();

	std::vector<int> res(id_count);
	for (int i = 0; i < id_count; i ++)
		res[i] = m_idMapDb->getPlainMinId(first_id + i);

	return res;
}

std::vector<int> Server::getMinIds(std::vector<int> ids)
{
	std::vector<int> min_ids(ids.size());
	for (size_t i = 0; i < ids.size(); i ++)
		min_ids[i] = m_idMapDb->getPlainMinId(ids[i]);

	return min_ids;
}

void Server::handleGetMinIdArray()
{
	sendLongVector(getTphashMinIds(recvOid()));
}

void Server::handleGetMinIds()
{
	sendLongArray(getMinIds(recvLongVector()));
}

void Server::handleGetFirstId()
{
	// "tp_hash" is not the same as "oid", but we can still use
	// the class GitOid for tp_hashes.
	GitOid tp_hash = recvOid();

	std::pair<int, int> first_ids = m_firstIds->getFirstId(tp_hash);
	int first_id = first_ids.first;
	int id_count = first_ids.second;

	if (first_id == 0)
	{
		char tp_hash_str[GIT_OID_HEXSZ + 1];
		tp_hash_str[GIT_OID_HEXSZ] = '\0';
		git_oid_fmt(tp_hash_str, tp_hash.oid());
		printf("First ID not found for this tp_hash: %s\n", tp_hash_str);
	}

	// "first_id == 0" means that the given tp_hash is unknown.
	sendLong((uint32_t)first_id);
	sendLong((uint32_t)id_count);
}

void Server::handleGetFirstIdMulti()
{
	size_t count = (size_t)recvLong();
	for (size_t i = 0; i < count; i ++)
	{
		GitOid tp_hash = recvOid();

		std::pair<int, int> first_ids = m_firstIds->getFirstId(tp_hash);
		int first_id = first_ids.first;
		int id_count = first_ids.second;

		if (first_id == 0)
		{
			char tp_hash_str[GIT_OID_HEXSZ + 1];
			tp_hash_str[GIT_OID_HEXSZ] = '\0';
			git_oid_fmt(tp_hash_str, tp_hash.oid());
			printf("First ID not found for this tp_hash: %s\n", tp_hash_str);
		}

		// "first_id == 0" means that the given tp_hash is unknown.
		sendLong((uint32_t)first_id);
		sendLong((uint32_t)id_count);
	}
}

void Server::handleInvolvedByMinIds()
{
	// read input data
	size_t tp_hash_count = recvLong();
	std::vector<GitOid> tp_hashes;
	for (size_t i = 0; i < tp_hash_count; i ++)
		tp_hashes.push_back(recvOid());

	std::vector<int> min_ids = getMinIds(recvLongVector());

	// process request
	sort(min_ids.begin(), min_ids.end());

	// TODO: sort tp_hashes by first_id (to optimize reads from mapped file), remember indices before sorting!

	std::vector<int> res;
	for (size_t i = 0; i < tp_hashes.size(); i ++)
	{
		std::vector<int> c_ids = getTphashMinIds(tp_hashes[i]);
		sort(c_ids.begin(), c_ids.end());

		std::vector<int> intersection(min_ids.size());
		if (set_intersection(
			min_ids.begin(), min_ids.end(),
			c_ids.begin(), c_ids.end(),
			intersection.begin()) != intersection.begin())
		{
			res.push_back(i);
		}
	}

	// write output data
	sendLongVector(res);
}

void Server::commandHandler()
{
	uint32_t command;
	try
	{
		command = recvLong();
	}
	catch (std::exception &e)
	{
		disconnect();
		return;
	}

	try
	{
		if (command == StupidsClient::CMD_EXIT)
		{
			flushToClient();
			disconnect();
			return;
		}
		else if (command == StupidsClient::CMD_GET_MIN_ID_ARRAY)
			handleGetMinIdArray();
		else if (command == StupidsClient::CMD_GET_FIRST_ID)
			handleGetFirstId();
		else if (command == StupidsClient::CMD_GET_FIRST_ID_MULTI)
			handleGetFirstIdMulti();
		else if (command == StupidsClient::CMD_GET_MIN_IDS)
			handleGetMinIds();
		else if (command == StupidsClient::CMD_INVOLVED_BY_MIN_IDS)
			handleInvolvedByMinIds();
		else
		{
			printf("Unknown command code %d.\n", command);
			flushErrorToClient(1);
			disconnect();
			return;
		}
	}
	catch (std::exception &e)
	{
		printf("Exception raised while processing client's requests:\n\t%s\n", e.what());
		flushErrorToClient(2);
		disconnect();
		return;
	}

	flushToClient();
}

//-----------------------------------------------------

const char *Server::ExceptionTpHashNotFound::what() const throw()
{
	return "ExceptionTpHashNotFound (could not find first_id for the given tp_hash)";
}

