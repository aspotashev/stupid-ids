
// http://www.gnu.org/software/gettext/manual/gettext.html#libgettextpo
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <vector>
#include <map>

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

std::vector<Message *> read_po_file_messages(const char *filename, bool loadObsolete)
{
	po_file_t file = po_file_read(filename);
	po_message_iterator_t iterator = po_message_iterator(file, "messages");

	// skipping header
	po_message_t message = po_next_message(iterator);

	std::vector<Message *> res;
	int index = 0;
	while (message = po_next_message(iterator))
	{
		// Checking that obsolete messages go after all normal messages
		assert(index == (int)res.size());

		if (loadObsolete || !po_message_is_obsolete(message))
			res.push_back(new Message(
				message,
				po_message_is_obsolete(message) ? -1 : index,
				filename));
		if (!po_message_is_obsolete(message))
			index ++;
	}

	// free memory
	po_file_free(file);

	return res;
}

void StupIdTranslationCollector::insertPo(const char *filename)
{
	std::string tp_hash = calculate_tp_hash(filename);
	std::vector<int> min_ids = get_min_ids_by_tp_hash(tp_hash.c_str());

	//--------------------- insert messages --------------------
	std::vector<Message *> messages = read_po_file_messages(filename, false);

	printf("id_count = %d\n", (int)min_ids.size());
	assert(messages.size() == min_ids.size());

	int index; // outside of the loop in order to calculate message count
	for (int index = 0; index < (int)messages.size(); index ++)
	{
		printf("min_ids[%d] = %d\n", index, min_ids[index]);

		if (m_trans.find(min_ids[index]) == m_trans.end())
		{
			std::pair<int, std::vector<Message *> > new_pair;
			new_pair.first = min_ids[index];
			new_pair.second = std::vector<Message *>();

			m_trans.insert(new_pair);
		}

		// fuzzy and untranslated messages will be also added
		m_trans[min_ids[index]].push_back(messages[index]);
	}
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

