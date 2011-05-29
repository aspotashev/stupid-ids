
#include <gettextpo-helper/translation-collector.h>
#include <gettextpo-helper/gettextpo-helper.h>
#include <gettextpo-helper/stupids-client.h>
#include <gettextpo-helper/translationcontent.h>

StupIdTranslationCollector::StupIdTranslationCollector()
{
	m_trans = std::map<int, MessageGroup *>();
	m_client = new StupidsClient();
}

StupIdTranslationCollector::~StupIdTranslationCollector()
{
	delete m_client;
}

// DEPRECATED
void StupIdTranslationCollector::insertPo(const char *filename)
{
	TranslationContent *content = new TranslationContent(filename);
	insertPo(content, filename);
	delete content;
}

void StupIdTranslationCollector::insertPo(TranslationContent *content, const char *filename)
{
	std::string tp_hash = content->calculateTpHash();
	std::vector<int> min_ids = m_client->getMinIds(tp_hash.c_str());

	//--------------------- insert messages --------------------
	std::vector<MessageGroup *> messages = content->readMessages(filename, false);

//	printf("id_count = %d\n", (int)min_ids.size());
	assert(messages.size() == min_ids.size());

	int index; // outside of the loop in order to calculate message count
	for (int index = 0; index < (int)messages.size(); index ++)
	{
		// fuzzy and untranslated messages will be also added
		if (m_trans.find(min_ids[index]) == m_trans.end())
		{
			std::pair<int, MessageGroup *> new_pair;
			new_pair.first = min_ids[index];
			new_pair.second = messages[index];

			m_trans.insert(new_pair);
		}
		else
		{
			m_trans[min_ids[index]]->mergeMessageGroup(messages[index]);
		}
	}
}

// Takes ownership of the buffer.
void StupIdTranslationCollector::insertPo(const void *buffer, size_t len, const char *filename)
{
	TranslationContent *content = new TranslationContent(buffer, len);
	insertPo(content, filename);
	delete content; // this also "delete[]"s the buffer
}

// Returns 'true' if there are different translations of the message.
//
// Cannot be 'const', because there is no const 'std::map::operator []'.
bool StupIdTranslationCollector::conflictingTrans(int min_id)
{
	assert(m_trans[min_id]->size() > 0);

	Message *msg = m_trans[min_id]->message(0);
	for (int i = 1; i < m_trans[min_id]->size(); i ++)
		if (!msg->equalTranslations(m_trans[min_id]->message(i)))
			return true;

	return false;
}

// Returns an array of 'min_id's.
//
// Cannot be 'const', because there is no const 'std::map::operator []'.
std::vector<int> StupIdTranslationCollector::listConflicting()
{
	std::vector<int> res;

	for (std::map<int, MessageGroup *>::iterator iter = m_trans.begin();
		iter != m_trans.end();
		iter ++)
	{
		if (conflictingTrans(iter->first))
			res.push_back(iter->first);
	}

	return res;
}

MessageGroup *StupIdTranslationCollector::listVariants(int min_id)
{
	assert (m_trans.find(min_id) != m_trans.end());

	return m_trans[min_id];
}

int StupIdTranslationCollector::numSharedIds() const
{
	int res = 0;
	for (std::map<int, MessageGroup *>::const_iterator iter = m_trans.begin();
		iter != m_trans.end();
		iter ++)
	{
		if (iter->second->size() > 1)
			res ++;
	}

	return res;
}

int StupIdTranslationCollector::numIds() const
{
	return (int)m_trans.size();
}

