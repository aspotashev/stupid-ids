
#include <string>
#include <vector>
#include <sstream>

#include <gettext-po.h>

class Message;
class MessageGroup;

// TODO: derive class Message from this class
class IddiffMessage
{
public:
	IddiffMessage();
	IddiffMessage(po_message_t message);
	~IddiffMessage();

	bool isFuzzy() const;
	void setFuzzy(bool fuzzy);
	int numPlurals() const;
	const char *msgstr(int plural_form) const;

	void setMsgstr(int index, const char *str);
	void addMsgstr(const char *str);

	std::string formatPoMessage() const;

	bool equalTranslations(const IddiffMessage *message) const;

	bool equalTranslations(const Message *message) const;
	void copyTranslationsToMessage(Message *message);

protected:
	static std::string formatString(const char *str);

private:
	const static int MAX_PLURAL_FORMS = 4; // increase this if you need more plural forms

	int m_numPlurals; // =1 if the message does not use plural forms
	char *m_msgstr[MAX_PLURAL_FORMS];
	bool m_fuzzy;
};

//----------------------------------------------

class TranslationContent;
class StupIdTranslationCollector;

class Iddiffer
{
public:
	Iddiffer();
	~Iddiffer();

	void diffAgainstEmpty(TranslationContent *content_b);
	void diffFiles(TranslationContent *content_a, TranslationContent *content_b);
	void loadIddiff(const char *filename);

	void minimizeIds();
	std::vector<int> involvedIds();

	std::string generateIddiffText();

	std::vector<IddiffMessage *> getIddiffArr(std::vector<std::pair<int, IddiffMessage *> > &section, int msg_id);
	std::vector<IddiffMessage *> getRemovedArr(int msg_id);
	std::vector<IddiffMessage *> getAddedArr(int msg_id);
	IddiffMessage *getAdded(int msg_id);

	void applyToMessage(MessageGroup *messageGroup, int min_id);
	void applyToContent(TranslationContent *content);
	void applyIddiff(StupIdTranslationCollector *collector);

	void insertRemoved(std::pair<int, IddiffMessage *> item);
	void insertAdded(std::pair<int, IddiffMessage *> item);
	void merge(Iddiffer *diff);

	// TODO: may be remove this?
	static std::string generateIddiffText(TranslationContent *content_a, TranslationContent *content_b);

protected:
	void writeMessageList(std::vector<std::pair<int, IddiffMessage *> > list);
	void loadMessageListEntry(const char *line, std::vector<std::pair<int, IddiffMessage *> > &list);

	static std::string formatPoMessage(po_message_t message);

private:
	std::string m_subject;
	std::string m_author;
	// TODO: m_date

	std::vector<std::pair<int, IddiffMessage *> > m_removedList;
	std::vector<std::pair<int, IddiffMessage *> > m_addedList;
	bool m_minimizedIds;

	std::ostringstream m_output;
};

