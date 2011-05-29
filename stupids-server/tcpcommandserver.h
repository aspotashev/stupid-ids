
#include <sys/socket.h>
#include <netinet/in.h>

class TcpCommandServer
{
public:
	TcpCommandServer();
	~TcpCommandServer();

	// Enters an infinite loop
	void start();

protected:
	virtual void commandHandler(const char *command) = 0;
	void sendToClient(const void *data, size_t len);
	void disconnect();

private:
	void sessionOpened();


	int m_connfd;
	struct sockaddr_in m_cliaddr;
	socklen_t m_clilen;

	bool m_closeConnection;
};
