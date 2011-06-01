
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <vector>

#include <gettextpo-helper/gettextpo-helper.h>
#include <gettextpo-helper/stupids-client.h>
#include <gettextpo-helper/translationcontent.h>

// TODO: derive class Message from this class
class IddiffMessage
{
public:
	IddiffMessage(po_message_t message);
	~IddiffMessage();

	bool isFuzzy() const;
	int numPlurals() const;
	const char *msgstr(int plural_form) const;

	void setMsgstr(int index, const char *str);

	std::string formatPoMessage() const;

protected:
	static std::string formatString(const char *str);

private:
	const static int MAX_PLURAL_FORMS = 4; // increase this if you need more plural forms

	int m_numPlurals; // =1 if the message does not use plural forms
	char *m_msgstr[MAX_PLURAL_FORMS];
	bool m_fuzzy;
};

IddiffMessage::IddiffMessage(po_message_t message)
{
	for (int i = 0; i < MAX_PLURAL_FORMS; i ++)
		m_msgstr[i] = 0;

	// set m_numPlurals
	m_numPlurals = po_message_n_plurals(message);
	bool m_plural;
	if (m_numPlurals == 0) // message does not use plural forms
	{
		m_numPlurals = 1;
		m_plural = false;
	}
	else
	{
		m_plural = true;
	}
	assert(m_numPlurals <= MAX_PLURAL_FORMS); // limited by the size of m_msgstr


	for (int i = 0; i < m_numPlurals; i ++)
	{
		const char *tmp;
		if (m_plural)
		{
			tmp = po_message_msgstr_plural(message, i);
		}
		else
		{
			assert(i == 0); // there can be only one form if 'm_plural' is false

			tmp = po_message_msgstr(message);
		}

		setMsgstr(i, tmp);
	}

	m_fuzzy = po_message_is_fuzzy(message) != 0;
}

IddiffMessage::~IddiffMessage()
{
	// TODO: free memory
}

void IddiffMessage::setMsgstr(int index, const char *str)
{
	assert(m_msgstr[index] == 0);

	m_msgstr[index] = xstrdup(str);
}

bool IddiffMessage::isFuzzy() const
{
	return m_fuzzy;
}

int IddiffMessage::numPlurals() const
{
	return m_numPlurals;
}

const char *IddiffMessage::msgstr(int plural_form) const
{
	assert(plural_form >= 0 && plural_form < m_numPlurals);

	return m_msgstr[plural_form];
}

std::string IddiffMessage::formatPoMessage() const
{
	std::string res;

	if (isFuzzy())
		res += "f";

	res += formatString(msgstr(0));
	for (int i = 1; i < numPlurals(); i ++)
	{
		res += " "; // separator
		res += formatString(msgstr(i));
	}

	return res;
}

// escape quotes (" -> \") and put in quotes
std::string IddiffMessage::formatString(const char *str)
{
	assert(str);

	std::string res;

	res += "\""; // opening quote

	size_t len = strlen(str);
	for (size_t i = 0; i < len; i ++)
	{
		if (str[i] == '\"')
			res += "\\\""; // escape quote
		else
			res += str[i];
	}

	res += "\"";
	return res;
}

//----------------------------------------------

class Iddiff
{
};

//----------------------------------------------

class Iddiffer
{
public:
	Iddiffer();
	~Iddiffer();

	void generateIddiff(TranslationContent *content_a, TranslationContent *content_b);

	// TODO: make this a static function in class Iddiff, and remove class Iddiffer
	std::string generateIddiffText(TranslationContent *content_a, TranslationContent *content_b);

protected:
	void writeMessageList(std::vector<std::pair<int, IddiffMessage *> > list);

	static std::string formatPoMessage(po_message_t message);

private:
	std::string m_subject;
	std::string m_author;
	// TODO: m_date

	std::vector<std::pair<int, IddiffMessage *> > m_removedList;
	std::vector<std::pair<int, IddiffMessage *> > m_addedList;

	std::string m_output;
};

Iddiffer::Iddiffer()
{
}

Iddiffer::~Iddiffer()
{
}

std::string Iddiffer::formatPoMessage(po_message_t message)
{
	IddiffMessage *idm = new IddiffMessage(message);
	std::string res = idm->formatPoMessage();
	delete idm;

	return res;
}

void Iddiffer::writeMessageList(std::vector<std::pair<int, IddiffMessage *> > list)
{
	for (size_t i = 0; i < list.size(); i ++)
	{
		// TODO: do this using std::stringstream?
		char id_str[20];
		sprintf(id_str, "%d", list[i].first);

		m_output += std::string(id_str);
		m_output += std::string(" ");
		m_output += list[i].second->formatPoMessage();
		m_output += std::string("\n");
	}
}

// This function fills m_removedList and m_addedList
void Iddiffer::generateIddiff(TranslationContent *content_a, TranslationContent *content_b)
{
	m_removedList.clear();
	m_addedList.clear();

	// .po files should be derived from the same .pot
	const git_oid *tp_hash = content_a->calculateTpHash();
	assert(git_oid_cmp(tp_hash, content_b->calculateTpHash()) == 0);

	// first_id is the same for 2 files
	StupidsClient *client = new StupidsClient();
	int first_id = client->getFirstId(tp_hash);
	delete client;


	// compare pairs of messages in 2 .po files
	po_file_t file_a = content_a->poFileRead();
	po_file_t file_b = content_b->poFileRead();

	po_message_iterator_t iterator_a =
		po_message_iterator(file_a, "messages");
	po_message_iterator_t iterator_b =
		po_message_iterator(file_b, "messages");
	// skipping headers
	po_message_t message_a = po_next_message(iterator_a);
	po_message_t message_b = po_next_message(iterator_b);

	for (int index = 0;
		(message_a = po_next_message(iterator_a)) &&
		(message_b = po_next_message(iterator_b)) &&
		!po_message_is_obsolete(message_a) &&
		!po_message_is_obsolete(message_b);
		index ++)
	{
		if (strcmp(po_message_comments(message_a), po_message_comments(message_b)))
		{
			printf("Changes in comments will be ignored!");
			printf("<<<<<\n");
			printf("%s", po_message_comments(message_a)); // "\n" should be included in comments
			printf("=====\n");
			printf("%s", po_message_comments(message_b)); // "\n" should be included in comments
			printf(">>>>>\n");
		}


		// Messages can be:
		//     "" -- untranslated (does not matter fuzzy or not, so f"" is illegal)
		//     "abc" -- translated
		//     f"abc" -- fuzzy

		// Types of possible changes:
		//     ""     -> ""     : -
		//     ""     -> "abc"  : ADDED
		//     ""     -> f"abc" : - (fuzzy messages are "weak", you should write in comments instead what you are not sure in)
		//     "abc"  -> ""     : REMOVED
		//     "abc"  -> "abc"  : -
		//     "abc"  -> f"abc" : REMOVED
		//     f"abc" -> ""     : REMOVED (removing fuzzy messages is OK)
		//     f"abc" -> "abc"  : REMOVED, ADDED
		//     f"abc" -> f"abc" : -
		//     "abc"  -> "def"  : REMOVED, ADDED
		//     "abc"  -> f"def" : REMOVED
		//     f"abc" -> "def"  : REMOVED, ADDED
		//     f"abc" -> f"def" : REMOVED

		// If there were no changes (i.e. message_a == message_b)
		if (!compare_po_message_msgstr(message_a, message_b))
			continue;

		// Adding to "REMOVED" if:
		//    "A" is not untranslated & there were changes (i.e. message_a != message_b)
		if (!po_message_is_untranslated(message_a))
			m_removedList.push_back(std::pair<int, IddiffMessage *>(
				first_id + index, new IddiffMessage(message_a)));

		// Adding to "ADDED" if:
		//    "B" is translated & there were changes (i.e. message_a != message_b)
		if (!po_message_is_untranslated(message_b) && !po_message_is_fuzzy(message_b))
			m_addedList.push_back(std::pair<int, IddiffMessage *>(
				first_id + index, new IddiffMessage(message_b)));
	}

	// free memory
	po_message_iterator_free(iterator_a);
	po_message_iterator_free(iterator_b);
	po_file_free(file_a);
	po_file_free(file_b);
}

std::string Iddiffer::generateIddiffText(TranslationContent *content_a, TranslationContent *content_b)
{
	generateIddiff(content_a, content_b);

	m_output.clear();
	if (m_removedList.size() > 0 || m_addedList.size() > 0)
	{
		m_output += std::string("Subject: \n");
		m_output += std::string("Author: \n");
		m_output += std::string("Date: \n");
		m_output += std::string("\n");
		m_output += std::string("REMOVED\n");
		writeMessageList(m_removedList);
		m_output += std::string("ADDED\n");
		writeMessageList(m_addedList);
	}

	return m_output;
}

//----------------------------------------------

int main(int argc, char *argv[])
{
	assert(argc == 3); // 2 arguments

	TranslationContent *content_a = new TranslationContent(argv[1]);
	TranslationContent *content_b = new TranslationContent(argv[2]);

	Iddiffer *differ = new Iddiffer();
	std::cout << differ->generateIddiffText(content_a, content_b);

	return 0;
}

