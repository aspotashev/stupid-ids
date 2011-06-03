
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <map>

#include <gettextpo-helper/detectorbase.h>
#include <gettextpo-helper/mappedfile.h>
#include <gettextpo-helper/stupids-client.h>

#include "tcpcommandserver.h"

class FiledbFirstIds
{
public:
	FiledbFirstIds(const char *filename, const char *filename_next_id);
	~FiledbFirstIds();

	std::pair<int, int> getFirstId(const GitOid &tp_hash);

private:
	std::map<GitOid, std::pair<int, int> > m_firstIds;
};

FiledbFirstIds::FiledbFirstIds(const char *filename, const char *filename_next_id)
{
	FILE *f = fopen(filename, "r");
	assert(f);

	std::vector<GitOid> oids;
	std::vector<int> first_ids;

	char line[60];
	while (fgets(line, 60, f))
	{
		// tp_hash(40) + space(1) + first_id(1)
		assert(strlen(line) >= 42);
		// space between tp_hash and first_id
		assert(line[40] == ' ');

		line[40] = '\0'; // truncate at the end of tp_hash
		oids.push_back(GitOid(line));
		first_ids.push_back(atoi(line + 41));
	}
	fclose(f);

	// Read next_id.txt
	f = fopen(filename_next_id, "r");
	assert(f);

	assert(fgets(line, 60, f));
	int next_id = atoi(line);

	fclose(f);


	first_ids.push_back(next_id);

	for (size_t i = 0; i < oids.size(); i ++)
		m_firstIds[oids[i]] = std::make_pair<int, int>(
			first_ids[i], first_ids[i + 1] - first_ids[i]);
}

FiledbFirstIds::~FiledbFirstIds()
{
}

std::pair<int, int> FiledbFirstIds::getFirstId(const GitOid &tp_hash)
{
	std::map<GitOid, std::pair<int, int> >::const_iterator iter;
	iter = m_firstIds.find(tp_hash);

	if (iter != m_firstIds.end())
		return iter->second;
	else
		return std::make_pair<int, int>(0, 0); // tp_hash not found
}

//-------------------

class Server : public TcpCommandServer
{
public:
	Server();
	~Server();

	virtual void commandHandler(uint32_t command);

private:
	GitOid recvOid();

	void handleGetMinIds();
	void handleGetFirstId();


	FiledbFirstIds *m_firstIds;
	MappedFileIdMapDb *m_idMapDb;
};

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

void Server::handleGetMinIds()
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
		int min_id = m_idMapDb->getRecursiveMinId(first_id + i);
		output[i + 1] = htonl((uint32_t)min_id);
	}

	sendToClient(output, sizeof(uint32_t) * (id_count + 1));

	delete [] output;
}

void Server::handleGetFirstId()
{
	// "tp_hash" is not the same as "oid", but we can still use
	// the class GitOid for tp_hashes.
	GitOid tp_hash = recvOid();

	std::pair<int, int> first_ids = m_firstIds->getFirstId(tp_hash);
	int first_id = first_ids.first;
	assert(first_id != 0);

	uint32_t output = htonl((uint32_t)first_id);
	sendToClient(&output, sizeof(uint32_t));
}

void Server::commandHandler(uint32_t command)
{
	if (command == StupidsClient::CMD_EXIT)
		disconnect();
	else if (command == StupidsClient::CMD_GET_MIN_ID_ARRAY)
		handleGetMinIds();
	else if (command == StupidsClient::CMD_GET_FIRST_ID)
		handleGetFirstId();
	else
	{
		printf("Unknown command code %d.\n", command);
	}
}

//-------------------

int main()
{
	Server server;
	server.start();

	return 0;
}

