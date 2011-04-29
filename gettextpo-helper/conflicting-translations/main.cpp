
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

// Returns 'true' when msgstr (or all msgstr[i]) are empty.
bool po_message_is_untranslated(po_message_t message)
{
	if (po_message_msgstr_plural(message, 0)) // message with plurals
	{
		for (int i = 0; po_message_msgstr_plural(message, i); i ++)
			if (po_message_msgstr_plural(message, i)[0] != '\0') // non-empty string found
				return false; // not untranslated (i.e. translated or fuzzy)

		return true; // all strings are empty
	}
	else // message without plurals
	{
		return po_message_msgstr(message)[0] == '\0';
	}
}

int po_message_n_plurals(po_message_t message)
{
	int i;
	for (i = 0; po_message_msgstr_plural(message, i); i ++);

	return i;
}

// Returns 0 when msgstr (or all msgstr[i]) are the same in two messages.
int compare_po_message_msgstr(po_message_t message_a, po_message_t message_b)
{
	int n_plurals = po_message_n_plurals(message_a);
	assert(n_plurals == po_message_n_plurals(message_b));

	if (n_plurals > 0) // messages with plurals
	{
		for (int i = 0; i < n_plurals; i ++)
			if (strcmp(
				po_message_msgstr_plural(message_a, i),
				po_message_msgstr_plural(message_b, i)))
				return 1; // differences found

		return 0; // all strings are equal
	}
	else // message without plurals
	{
		return strcmp(po_message_msgstr(message_a), po_message_msgstr(message_b));
	}
}

class Message
{
public:
	Message(po_message_t message, int index, const char *filename);
	~Message();

	int index() const;
	const char *filename() const;

private:
//	bool m_plural;
//	char *m_msgstr;
//	char *m_msgstrPlural;
//
//	bool fuzzy;

	int m_index;
	const char *m_filename;
};

Message::Message(po_message_t message, int index, const char *filename):
	m_index(index),
	m_filename(filename)
{
}

Message::~Message()
{
}

int Message::index() const
{
	return m_index;
}

const char *Message::filename() const
{
	return m_filename;
}

class StupIdTranslationCollector
{
public:
	StupIdTranslationCollector();
	~StupIdTranslationCollector();

	void insertPo(const char *filename);

private:
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

void get_min_ids_by_tp_hash(const char *tp_hash, std::vector<int> &res)
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

//	std::vector<int> res; // TODO: reserve memory for 'id_count' elements

	for (int i = 0; i < id_count; i ++)
		res.push_back(fd_read_integer_from_line(sockfd));

//	return res;
}

void StupIdTranslationCollector::insertPo(const char *filename)
{
	std::string tp_hash = calculate_tp_hash(filename);
	std::vector<int> min_ids;
	get_min_ids_by_tp_hash(tp_hash.c_str(), min_ids);

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
			new_pair.first = 0;//min_ids[index];
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

int main(int argc, char *argv[])
{
	StupIdTranslationCollector collector;
	collector.insertPo("a1.po");
	collector.insertPo("a2.po");

	return 0;
}

