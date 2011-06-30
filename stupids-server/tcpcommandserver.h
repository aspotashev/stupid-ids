
#include <exception>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>

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

	/**
	* @brief Receives of block of data from client.
	*
	* May throw exceptions if client disconnects.
	*
	* @param data Pointer to the buffer to save data to.
	* @param len Length of data to be read (in bytes).
	**/
	void recvFromClient(void *data, size_t len);

	/**
	* @brief Prepares a block of data for sending to client.
	*
	* Data will be actually sent to client by flushToClient().
	*
	* @param data Pointer to data buffer.
	* @param len Length of data in buffer in bytes.
	**/
	void sendToClient(const void *data, size_t len);

	/**
	* @brief Sends data prepared by send*() calls to the client.
	*
	* Output buffer will be cleared afterwards.
	**/
	void flushToClient();

	/**
	* @brief Discards data prepared for sending to client
	* and sends the given error code instead.
	*
	* Output buffer will be cleared afterwards.
	**/
	void flushErrorToClient(uint32_t code);

	void disconnect();

private:
	void sessionOpened();

	/**
	* @brief Actually writes a data block to client.
	*
	* Internal use only, derived classes should use
	* flushToClient() or flushErrorToClient().
	*
	* @param data Pointer to data buffer.
	* @param len Length of data in buffer in bytes.
	**/
	void writeToClient(const void *data, size_t len);


	int m_connfd;
	struct sockaddr_in m_cliaddr;
	socklen_t m_clilen;

	bool m_closeConnection;

	std::string m_outputBuffer;
};

