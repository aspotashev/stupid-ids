
#include <string>
#include <vector>
#include <sstream>

#include <gettext-po.h>

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

class Iddiffer
{
public:
	Iddiffer();
	~Iddiffer();

	void diffFiles(TranslationContent *content_a, TranslationContent *content_b);
	void loadIddiff(const char *filename);

	std::string generateIddiffText();

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

	std::ostringstream m_output;
};

