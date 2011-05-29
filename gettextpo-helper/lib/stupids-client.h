
#include <exception>

//-------- Working with stupids-server.rb over TCP/IP --------

class TpHashNotFoundException : public std::exception
{
	virtual const char *what() const throw();
};

class StupidsClient
{
public:
	StupidsClient();
	~StupidsClient();

	std::vector<int> getMinIds(const char *tp_hash);
	int getFirstId(const char *tp_hash);

protected:
	void sockReadBlock(void *buffer, int length);
	void connect();
	void disconnect();
	void sendString(const char *str);

private:
	int m_sockfd;
};

