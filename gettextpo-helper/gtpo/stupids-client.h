#ifndef STUPIDSCLIENT_H
#define STUPIDSCLIENT_H

#include <arpa/inet.h>
#include <exception>
#include <vector>

#include <git2.h>

#include <gtpo/detectorbase.h>

/**
 * \brief Class for working with stupids-server over TCP/IP
 */
class StupidsClient
{
public:
    /**
     * \brief Exception raised when the function ::connect fails to connect to the server
     */
    class ExceptionConnectionFailed : public std::exception
    {
        virtual const char *what() const throw();
    };

    /**
     * \brief Exception raised when trying to send/receive data to/from the server, but the connection has already been closed
     */
    class ExceptionNoConnection : public std::exception
    {
        virtual const char *what() const throw();
    };

    /**
     * \brief Exception raised when stupids-server returns non-zero error code in reply.
     */
    class ExceptionRequestFailed : public std::exception
    {
        virtual const char *what() const throw();
    };

    /**
     * \brief Constructs a StupidsClient, but does not connect to a server
     */
    StupidsClient();

    ~StupidsClient();

    /**
     * \brief Sends the CMD_GET_MIN_ID_ARRAY request to the server and returns the results
     *
     * \param tp_hash Template-part hash of a translation file
     *
     * \returns Vector of minimized IDs of all messages from that translation file. IDs in the
     * vector go in the same order as the messages in translation file. The size of the
     * vector is equal to the number of messages in translation file.
     */
    std::vector<int> getMinIds(const git_oid *tp_hash);

    /**
     * \brief Sends the CMD_GET_MIN_IDS request to the server and returns the results
     *
     * \param msg_ids Vector of IDs you want to calculate minimized IDs for
     *
     * \returns Vector of minimized IDs for the given IDs. IDs in the output vector go
     * in the same order as in the input vector.
     */
    std::vector<int> getMinIds(std::vector<int> msg_ids);

    std::pair<int, int> getFirstIdPair(const git_oid *tp_hash);

    std::vector<std::pair<int, int> > getFirstIdPairs(std::vector<GitOid> tp_hashes);

    /**
     * \brief Sends the CMD_GET_FIRST_ID request to the server and returns the results
     *
     * \param tp_hash Template-part hash of a translation file
     *
     * \returns Non-minimized ID of the first message in the translation file. Non-minimized
     * IDs of the next messages from the translation file can be obtained by incrementing
     * this value. \n
     * For more understanding: the CMD_GET_MIN_ID_ARRAY[tp_hash->min_ids] command can
     * be implemented as follows: \n
     * 1. run CMD_GET_FIRST_ID command, the result being "first_id", \n
     * 2. create a vector with consequent numbers from range [first_id..first_id+num_messages-1],
     * where "num_messages" is the number of messages in the translation file, \n
     * 3. run CMD_GET_MIN_IDS on that vector.
     */
    int getFirstId(const git_oid *tp_hash);

    /**
     * \brief Sends the CMD_INVOLVED_BY_MIN_IDS request to the server and returns the results
     *
     * \param tp_hashes Template-part hashes of translation files
     * \param ids IDs that you want to find in those files (not necessarily minimized)
     *
     * \returns Vector of indices in @p tp_hashes of those translation files that contain
     * messages with any of the given IDs (IDs will be firstly minimized).
     * The size of the returned vector can be between zero and the size of @p tp_hashes.
     */
    std::vector<int> involvedByMinIds(std::vector<const git_oid *> tp_hashes, std::vector<int> ids);

    static StupidsClient &instance();

    enum
    {
        CMD_EXIT = 1,
        CMD_GET_MIN_ID_ARRAY = 2,
        CMD_GET_FIRST_ID = 3,
        CMD_GET_FIRST_ID_MULTI = 4,
        CMD_GET_MIN_IDS = 5,
        CMD_INVOLVED_BY_MIN_IDS = 6,
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
    void checkRecvErrorCode();

private:
    int m_sockfd;

    static StupidsClient *s_instance;
};

#define stupidsClient (StupidsClient::instance())

#endif // STUPIDSCLIENT_H

