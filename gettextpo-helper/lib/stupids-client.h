
#include <exception>
#include <arpa/inet.h>

#include <git2.h>

//-------- Working with stupids-server.rb over TCP/IP --------

class StupidsClient
{
public:
	StupidsClient();
	~StupidsClient();

	std::vector<int> getMinIds(const git_oid *tp_hash);
	int getFirstId(const git_oid *tp_hash);

	enum
	{
		CMD_EXIT = 1,
		CMD_GET_MIN_ID_ARRAY = 2,
		CMD_GET_FIRST_ID = 3
	};

protected:
	void sockReadBlock(void *buffer, int length);
	void connect();
	void disconnect();
	void sendLong(uint32_t data);
	void sendOid(const git_oid *oid);
	void sendToServer(const void *data, size_t len);

private:
	int m_sockfd;
};

