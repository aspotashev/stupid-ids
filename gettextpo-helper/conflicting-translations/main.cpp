
// http://www.gnu.org/software/gettext/manual/gettext.html#libgettextpo
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <vector>
#include <map>

// for working with stupids-server
#include <arpa/inet.h>
#include <sys/socket.h>

#include "../include/gettextpo-helper.h"


class StupIdTranslationCollector
{
public:
	StupIdTranslationCollector();
	~StupIdTranslationCollector();

	void insertPo(const char *filename);

	// Cannot be 'const', because there is no const 'std::map::operator []'.
	std::vector<int> listConflicting();

protected:
	// Cannot be 'const', because there is no const 'std::map::operator []'.
	bool conflictingTrans(int min_id);

private:
	// int -- min_id
	// std::vector<Message *> -- array of messages that belong to this 'min_id'
	std::map<int, std::vector<Message *> > m_trans;
};

StupIdTranslationCollector::StupIdTranslationCollector()
{
	m_trans = std::map<int, std::vector<Message *> >();
}

StupIdTranslationCollector::~StupIdTranslationCollector()
{
}

char *fd_read_line(int fd)
{
	static char buf[256];
	char c = '\0';

	int i;
	for (i = 0; i < 256; i ++)
	{
		assert(read(fd, &c, 1) == 1);

		if (c == '\n')
		{
			buf[i] = '\0'; // remove '\n'
			break;
		}
		else
		{
			buf[i] = c;
		}
	}
	// i == string length (without '\0')

	assert(buf[i] == '\0');

	char *res = new char [i + 1];
	memcpy(res, buf, i + 1);
	return res;
}

// This function assumes that there is only one integer number on the line.
int fd_read_integer_from_line(int fd)
{
	char *str = fd_read_line(fd);
	int res = atoi(str);
	delete [] str;

	return res;
}

std::vector<int> get_min_ids_by_tp_hash(const char *tp_hash)
{
	// initialize connection
	struct sockaddr_in servaddr;
	int sockfd;

	// SOCK_NONBLOCK requires Linux 2.6.27
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		assert(0);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(1234);
	inet_aton("127.0.0.1", &servaddr.sin_addr);

	if (connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0)
	{
		close(sockfd);

		printf("Could not connect to stupids-server\n");
		assert(0);
	};

	// send command
	const char get_min_ids_cmd[] = "get_min_id_array ";
	const char newline[] = "\n";
	assert(strlen(tp_hash) == 40); // sha-1 is 20 bytes long

	write(sockfd, get_min_ids_cmd, strlen(get_min_ids_cmd));
	write(sockfd, tp_hash, 40);
	write(sockfd, newline, strlen(newline));

	// read results
	char *id_count_str = fd_read_line(sockfd);
	if (!strcmp(id_count_str, "NOTFOUND"))
	{
		printf("tp_hash not found (%s)\n", tp_hash);
		assert(0);
	}

	int id_count = atoi(id_count_str);
	delete [] id_count_str;

	std::vector<int> res; // TODO: reserve memory for 'id_count' elements
	for (int i = 0; i < id_count; i ++)
		res.push_back(fd_read_integer_from_line(sockfd));

	return res;
}

void StupIdTranslationCollector::insertPo(const char *filename)
{
	std::string tp_hash = calculate_tp_hash(filename);
	std::vector<int> min_ids = get_min_ids_by_tp_hash(tp_hash.c_str());

	//--------------------- insert messages --------------------
	po_file_t file = po_file_read(filename);

	po_message_iterator_t iterator = po_message_iterator(file, "messages");

	// skipping header
	po_message_t message = po_next_message(iterator);

	printf("id_count = %d\n", (int)min_ids.size());

	int index; // outside of the loop in order to calculate message count
	for (index = 0;
		(message = po_next_message(iterator)) &&
		!po_message_is_obsolete(message);
		index ++)
	{
		assert(index < (int)min_ids.size()); // for safety

		printf("min_ids[%d] = %d\n", index, min_ids[index]);

		if (m_trans.find(min_ids[index]) == m_trans.end())
		{
			std::pair<int, std::vector<Message *> > new_pair;
			new_pair.first = min_ids[index];
			new_pair.second = std::vector<Message *>();

			m_trans.insert(new_pair);
		}

		// fuzzy and untranslated messages will be also added
		m_trans[min_ids[index]].push_back(new Message(message, index, filename));
	}

	assert(index == (int)min_ids.size());

	// free memory
	po_file_free(file);
}

// Cannot be 'const', because there is no const 'std::map::operator []'.
bool StupIdTranslationCollector::conflictingTrans(int min_id)
{
	assert(m_trans[min_id].size() > 0);

	Message *msg = m_trans[min_id][0];
	for (size_t i = 1; i < m_trans[min_id].size(); i ++)
		if (!msg->equalTranslations(m_trans[min_id][i]))
			return true;

	return false;
}

// Returns an array of 'min_id's.
//
// Cannot be 'const', because there is no const 'std::map::operator []'.
std::vector<int> StupIdTranslationCollector::listConflicting()
{
	std::vector<int> res;

	for (std::map<int, std::vector<Message *> >::iterator iter = m_trans.begin();
		iter != m_trans.end();
		iter ++)
	{
		if (conflictingTrans(iter->first))
			res.push_back(iter->first);
	}

	return res;
}

int main(int argc, char *argv[])
{
	StupIdTranslationCollector collector;
	collector.insertPo("a1.po");
	collector.insertPo("a2.po");

	std::vector<int> list = collector.listConflicting();
	for (int i = 0; i < (int)list.size(); i ++)
		printf("%d\n", list[i]);

	return 0;
}

