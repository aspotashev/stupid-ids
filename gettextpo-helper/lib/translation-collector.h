
#include <vector>
#include <map>

class Message;
class MessageGroup;
class TranslationContent;

class StupIdTranslationCollector
{
public:
	StupIdTranslationCollector();
	~StupIdTranslationCollector();

	void insertPo(const char *filename);
	void insertPo(TranslationContent *content);
	void insertPo(const void *buffer, size_t len, const char *filename);

	// Cannot be 'const', because there is no const 'std::map::operator []'.
	std::vector<int> listConflicting();
	MessageGroup *listVariants(int min_id);

protected:
	// Cannot be 'const', because there is no const 'std::map::operator []'.
	bool conflictingTrans(int min_id);

private:
	// int -- min_id
	// std::vector<Message *> -- array of messages that belong to this 'min_id'
	std::map<int, MessageGroup *> m_trans;
};

