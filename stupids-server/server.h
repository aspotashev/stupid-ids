
#include <gettextpo-helper/detectorbase.h>

#include "tcpcommandserver.h"

class FiledbFirstIds;
class MappedFileIdMapDb;

class Server : public TcpCommandServer
{
public:
	class ExceptionTpHashNotFound : public std::exception
	{
		virtual const char *what() const throw();
	};


	Server();
	~Server();

	virtual void commandHandler();

private:
	GitOid recvOid();
	uint32_t recvLong();
	std::vector<int> recvLongVector();

	void sendLong(uint32_t data);
	void sendLongArray(std::vector<int> arr);
	void sendLongVector(std::vector<int> vec);

	std::vector<int> getTphashMinIds(GitOid tp_hash);
	std::vector<int> getMinIds(std::vector<int> ids);

	void handleGetMinIdArray();
	void handleGetFirstId();
	void handleGetMinIds();
	void handleInvolvedByMinIds();


	FiledbFirstIds *m_firstIds;
	MappedFileIdMapDb *m_idMapDb;
};

