
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

	/**
	* @brief Prepares a 32-bit value for sending to client.
	*
	* Conversion to network byte order will be done automatically.
	*
	* @param data Integer value to send.
	**/
	void sendLong(uint32_t data);

	/**
	* @brief Prepares an array of integers (without their count) for sending to client.
	*
	* Conversion to network byte order will be done automatically.
	*
	* @param arr Array to send.
	**/
	void sendLongArray(std::vector<int> arr);

	/**
	* @brief Prepares a vector of integers (count + data) for sending to client.
	*
	* Conversion to network byte order will be done automatically.
	*
	* @param vec Vector to send.
	**/
	void sendLongVector(std::vector<int> vec);

	std::vector<int> getTphashMinIds(GitOid tp_hash);
	std::vector<int> getMinIds(std::vector<int> ids);

	void handleGetMinIdArray();
	void handleGetFirstId();
	void handleGetFirstIdMulti();
	void handleGetMinIds();
	void handleInvolvedByMinIds();


	FiledbFirstIds *m_firstIds;
	MappedFileIdMapDb *m_idMapDb;
};

