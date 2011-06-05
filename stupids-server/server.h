
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
	std::vector<int> recvLongVector();

	void sendLong(uint32_t data);

	void handleGetMinIdArray();
	void handleGetFirstId();
	void handleGetMinIds();


	FiledbFirstIds *m_firstIds;
	MappedFileIdMapDb *m_idMapDb;
};

