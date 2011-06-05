
#include <algorithm>
#include <map>

#include <gettextpo-helper/detectorbase.h>

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

//-------------------

class MappedFileIdMapDb;

class Server : public TcpCommandServer
{
public:
	Server();
	~Server();

	virtual void commandHandler();

private:
	GitOid recvOid();

	void handleGetMinIdArray();
	void handleGetFirstId();
	void handleGetMinIds();


	FiledbFirstIds *m_firstIds;
	MappedFileIdMapDb *m_idMapDb;
};

