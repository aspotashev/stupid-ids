
#include <vector>
#include <map>

class Message;
class StupidsClient;
class TranslationContent;

class StupIdTranslationCollector
{
public:
	StupIdTranslationCollector();
	~StupIdTranslationCollector();

	void insertPo(const char *filename);
	void insertPo(TranslationContent *content, const char *filename);

	// Cannot be 'const', because there is no const 'std::map::operator []'.
	std::vector<int> listConflicting();
	std::vector<Message *> listVariants(int min_id);

	int numSharedIds() const;
	int numIds() const;

protected:
	// Cannot be 'const', because there is no const 'std::map::operator []'.
	bool conflictingTrans(int min_id);

private:
	// int -- min_id
	// std::vector<Message *> -- array of messages that belong to this 'min_id'
	std::map<int, std::vector<Message *> > m_trans;
	StupidsClient *m_client;
};

