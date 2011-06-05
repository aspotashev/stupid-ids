
#include <exception>
#include <sys/socket.h>
#include <netinet/in.h>

class TcpCommandServer
{
public:
	class ExceptionNoConnection : public std::exception
	{
		virtual const char *what() const throw();
	};

	class ExceptionDisconnected : public std::exception
	{
		virtual const char *what() const throw();
	};

	class ExceptionRecvFailed : public std::exception
	{
		virtual const char *what() const throw();
	};


	TcpCommandServer();
	~TcpCommandServer();

	// Enters an infinite loop
	void start();

protected:
	virtual void commandHandler() = 0;
	void recvFromClient(void *data, size_t len);
	void sendToClient(const void *data, size_t len);
	void disconnect();

private:
	void sessionOpened();


	int m_connfd;
	struct sockaddr_in m_cliaddr;
	socklen_t m_clilen;

	bool m_closeConnection;
};

