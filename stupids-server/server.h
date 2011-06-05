
#include <gettextpo-helper/detectorbase.h>

#include "tcpcommandserver.h"

class FiledbFirstIds;
class MappedFileIdMapDb;

class Server : public TcpCommandServer
{
public:
	Server();
	~Server();

	virtual void commandHandler();

private:
	GitOid recvOid();
	uint32_t recvLong();

	void handleGetMinIdArray();
	void handleGetFirstId();
	void handleGetMinIds();


	FiledbFirstIds *m_firstIds;
	MappedFileIdMapDb *m_idMapDb;
};

