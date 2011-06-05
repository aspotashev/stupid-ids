
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
	std::vector<int> getMinIds(std::vector<int> msg_ids);
	int getFirstId(const git_oid *tp_hash);

	static StupidsClient &instance();

	enum
	{
		CMD_EXIT = 1,
		CMD_GET_MIN_ID_ARRAY = 2,
		CMD_GET_FIRST_ID = 3,
		CMD_GET_MIN_IDS = 4,
	};

protected:
	void connect();
	void disconnect();

	void sendToServer(const void *data, size_t len);
	void sendLong(uint32_t data);
	void sendOid(const git_oid *oid);
	void sendLongVector(std::vector<int> vec);

	void recvFromServer(void *data, size_t len);
	uint32_t recvLong();
	std::vector<int> recvLongArray(size_t count);
	std::vector<int> recvLongVector();

private:
	int m_sockfd;

	static StupidsClient *s_instance;
};

#define stupidsClient (StupidsClient::instance())

