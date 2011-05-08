
#include <vector>
#include <map>

class Message;

class StupIdTranslationCollector
{
public:
	StupIdTranslationCollector();
	~StupIdTranslationCollector();

	void insertPo(const char *filename);

	// Cannot be 'const', because there is no const 'std::map::operator []'.
	std::vector<int> listConflicting();
	std::vector<Message *> listVariants(int min_id);

protected:
	// Cannot be 'const', because there is no const 'std::map::operator []'.
	bool conflictingTrans(int min_id);

private:
	// int -- min_id
	// std::vector<Message *> -- array of messages that belong to this 'min_id'
	std::map<int, std::vector<Message *> > m_trans;
};

