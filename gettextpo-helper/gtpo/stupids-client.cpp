
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <vector>

// for working with stupids-server
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#include "stupids-client.h"
#include "detectorbase.h"

//-------- Working with stupids-server.rb over TCP/IP --------

StupidsClient *StupidsClient::s_instance = NULL;

StupidsClient::StupidsClient()
{
    m_sockfd = -1;
}

StupidsClient::~StupidsClient()
{
}

void StupidsClient::connect()
{
    // If already connected
    if (m_sockfd >= 0)
        return;

    // initialize connection
    struct sockaddr_in servaddr;

    // SOCK_NONBLOCK requires Linux 2.6.27
    if ((m_sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        assert(0);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(1234);
    inet_aton("127.0.0.1", &servaddr.sin_addr);

    if (::connect(m_sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0)
    {
        close(m_sockfd);
        m_sockfd = -1;

        printf("Could not connect to stupids-server\n");
        throw ExceptionConnectionFailed();
    };
}

void StupidsClient::disconnect()
{
    sendLong(CMD_EXIT);

    close(m_sockfd);
    m_sockfd = -1;
}

//-----------------------

void StupidsClient::recvFromServer(void *data, size_t len)
{
    if (m_sockfd < 0)
        throw ExceptionNoConnection();

    size_t bytes_read = 0;

    while (bytes_read < len)
    {
        int res = read(m_sockfd, (char *)data + bytes_read, len - bytes_read);
        assert(res > 0);

        bytes_read += res;
    }
}

uint32_t StupidsClient::recvLong()
{
    uint32_t res = 0xffffffff;
    recvFromServer(&res, sizeof(uint32_t));
    return ntohl(res);
}

std::vector<int> StupidsClient::recvLongArray(size_t count)
{
    uint32_t *arr = new uint32_t[count];
    recvFromServer(arr, sizeof(uint32_t) * count);

    std::vector<int> res(count);
    for (size_t i = 0; i < count; i ++)
        res[i] = (int)ntohl(arr[i]);
    delete [] arr;

    return res;
}

std::vector<int> StupidsClient::recvLongVector()
{
    int count = (int)recvLong();
    assert(count >= 0);

    return recvLongArray(count);
}

//-----------------------

void StupidsClient::sendToServer(const void *data, size_t len)
{
    if (m_sockfd < 0)
        throw ExceptionNoConnection();

    size_t written = 0;
    while (written < len)
    {
        ssize_t res = write(m_sockfd, data, len);
        if (res <= 0)
        {
            printf("res = %d, len = %d, errno = %d\n", (int)res, (int)len, errno);
            assert(0);
        }

        written += res;
    }

    assert(written == len);
}

void StupidsClient::sendLong(uint32_t data)
{
    data = htonl(data);
    sendToServer(&data, 4);
}

void StupidsClient::sendOid(const GitOid& oid)
{
    assert(!oid.isNull());

    sendToServer(oid.oid(), GIT_OID_RAWSZ);
}

void StupidsClient::sendLongVector(std::vector<int> vec)
{
    size_t count = vec.size();
    uint32_t *arr = new uint32_t[count];

    sendLong((uint32_t)count);
    for (size_t i = 0; i < count; i ++)
        arr[i] = htonl((uint32_t)vec[i]);
    sendToServer(arr, count * 4);
    delete [] arr;
}

//-----------------------

std::vector<int> StupidsClient::getMinIds(const GitOid& tp_hash)
{
    assert(!tp_hash.isNull());

    connect();

    // send command
    sendLong(CMD_GET_MIN_ID_ARRAY);
    sendOid(tp_hash);

    checkRecvErrorCode();

    // read results
    return recvLongVector();

//  if (output_len >= 9 && !memcmp(output, "NOTFOUND\n", 9))
//  {
//      printf("tp_hash not found (%s)\n", tp_hash);
//      throw TpHashNotFoundException();
//  }
}

std::vector<int> StupidsClient::getMinIds(std::vector<int> msg_ids)
{
    connect();

    // send command
    sendLong(CMD_GET_MIN_IDS);
    sendLongVector(msg_ids);

    checkRecvErrorCode();

    // read results
    return recvLongArray(msg_ids.size());
}

std::pair<int, int> StupidsClient::getFirstIdPair(const GitOid& tp_hash)
{
    assert(!tp_hash.isNull());

    try
    {
        connect();
    }
    catch (ExceptionConnectionFailed &e)
    {
        return std::pair<int, int>(-1, -1);
    }

    // send command
    sendLong(CMD_GET_FIRST_ID);
    sendOid(tp_hash);

    checkRecvErrorCode();

    // read results
    int first_id = (int)recvLong();
    int id_count = (int)recvLong();

    return std::pair<int, int>(first_id, id_count);
}

int StupidsClient::getFirstId(const GitOid& tp_hash)
{
    std::pair<int, int> res = getFirstIdPair(tp_hash);
    return res.first; // first_id
}

std::vector<std::pair<int, int> > StupidsClient::getFirstIdPairs(std::vector<GitOid> tp_hashes)
{
    try
    {
        connect();
    }
    catch (ExceptionConnectionFailed &e)
    {
        return std::vector<std::pair<int, int> >();
    }

    // send command
    sendLong(CMD_GET_FIRST_ID_MULTI);
    size_t count = tp_hashes.size();
    sendLong(count);
    for (size_t i = 0; i < count; i ++)
        sendOid(tp_hashes[i].oid());

    checkRecvErrorCode();

    // read results
    std::vector<std::pair<int, int> > res;
    for (size_t i = 0; i < count; i ++)
    {
        int first_id = (int)recvLong();
        int id_count = (int)recvLong();
        res.push_back(std::pair<int, int>(first_id, id_count));
    }

    return res;
}

std::vector<int> StupidsClient::involvedByMinIds(std::vector<GitOid> tp_hashes, std::vector<int> ids)
{
    connect();

    // send command
    sendLong(CMD_INVOLVED_BY_MIN_IDS);

    // send "tp_hashes"
    // TODO: StupidsClient::sendOidVector
    sendLong(tp_hashes.size());
    for (size_t i = 0; i < tp_hashes.size(); i ++)
        sendOid(tp_hashes[i]);

    // send "ids"
    sendLongVector(ids);

    checkRecvErrorCode();

    // read results
    return recvLongVector();
}

StupidsClient &StupidsClient::instance()
{
    if (s_instance == NULL)
    {
        s_instance = new StupidsClient();
    }

    return *s_instance;
}

void StupidsClient::checkRecvErrorCode()
{
    if (recvLong() != 0) // checking error code
        throw ExceptionRequestFailed();
}

//--------------------------------------------

const char *StupidsClient::ExceptionConnectionFailed::what() const throw()
{
    return "ExceptionConnectionFailed (could not connect to stupids-server)";
}

const char *StupidsClient::ExceptionNoConnection::what() const throw()
{
    return "ExceptionNoConnection (connection has already been closed)";
}

const char* StupidsClient::ExceptionRequestFailed::what() const throw()
{
    return "ExceptionRequestFailed (stupids-server returned non-zero error code)";
}

