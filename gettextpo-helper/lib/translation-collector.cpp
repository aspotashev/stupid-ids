
#include "translation-collector.h"
#include "gettextpo-helper.h"
#include "stupids-client.h"

StupIdTranslationCollector::StupIdTranslationCollector()
{
	m_trans = std::map<int, std::vector<Message *> >();
}

StupIdTranslationCollector::~StupIdTranslationCollector()
{
}

void StupIdTranslationCollector::insertPo(const char *filename)
{
	std::string tp_hash = calculate_tp_hash(filename);
	std::vector<int> min_ids = get_min_ids_by_tp_hash(tp_hash.c_str());

	//--------------------- insert messages --------------------
	std::vector<Message *> messages = read_po_file_messages(filename, false);

//	printf("id_count = %d\n", (int)min_ids.size());
	assert(messages.size() == min_ids.size());

	int index; // outside of the loop in order to calculate message count
	for (int index = 0; index < (int)messages.size(); index ++)
	{
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

// Returns 'true' if there are different translations of the message.
//
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

std::vector<Message *> StupIdTranslationCollector::listVariants(int min_id)
{
	assert (m_trans.find(min_id) != m_trans.end());

	return m_trans[min_id];
}

int StupIdTranslationCollector::numSharedIds() const
{
	int res = 0;
	for (std::map<int, std::vector<Message *> >::const_iterator iter = m_trans.begin();
		iter != m_trans.end();
		iter ++)
	{
		if (iter->second.size() > 1)
			res ++;
	}

	return res;
}

int StupIdTranslationCollector::numIds() const
{
	return (int)m_trans.size();
}

