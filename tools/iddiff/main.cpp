
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

	// TODO: make this a static function in class Iddiff, and remove class Iddiffer
	std::string generateIddiffText(TranslationContent *content_a, TranslationContent *content_b);

protected:
	void writeMessageList(std::vector<std::pair<int, std::string> > list);

	static std::string formatString(const char *str);
	static std::string formatPoMessage(po_message_t message);

private:
	std::string m_output;
};

Iddiffer::Iddiffer()
{
}

Iddiffer::~Iddiffer()
{
}

// escape quotes (" -> \") and put in quotes
std::string Iddiffer::formatString(const char *str)
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

std::string Iddiffer::formatPoMessage(po_message_t message)
{
	IddiffMessage *idm = new IddiffMessage(message);

	std::string res;

	if (idm->isFuzzy())
		res += "f";

	res += formatString(idm->msgstr(0));
	for (int i = 1; i < idm->numPlurals(); i ++)
	{
		res += " "; // separator
		res += formatString(idm->msgstr(i));
	}


	delete idm;
	return res;
}

void Iddiffer::writeMessageList(std::vector<std::pair<int, std::string> > list)
{
	for (size_t i = 0; i < list.size(); i ++)
	{
		// TODO: do this using std::stringstream?
		char id_str[20];
		sprintf(id_str, "%d", list[i].first);

		m_output += std::string(id_str);
		m_output += std::string(" ");
		m_output += list[i].second;
		m_output += std::string("\n");
	}
}

std::string Iddiffer::generateIddiffText(TranslationContent *content_a, TranslationContent *content_b)
{
	m_output.clear();

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

	std::vector<std::pair<int, std::string> > removed_list;
	std::vector<std::pair<int, std::string> > added_list;

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
			removed_list.push_back(std::pair<int, std::string>(
				first_id + index, formatPoMessage(message_a)));

		// Adding to "ADDED" if:
		//    "B" is translated & there were changes (i.e. message_a != message_b)
		if (!po_message_is_untranslated(message_b) && !po_message_is_fuzzy(message_b))
			added_list.push_back(std::pair<int, std::string>(
				first_id + index, formatPoMessage(message_b)));
	}

	// free memory
	po_message_iterator_free(iterator_a);
	po_message_iterator_free(iterator_b);
	po_file_free(file_a);
	po_file_free(file_b);

	if (removed_list.size() > 0 || added_list.size() > 0)
	{
		m_output += std::string("Subject: \n");
		m_output += std::string("Author: \n");
		m_output += std::string("Date: \n");
		m_output += std::string("\n");
		m_output += std::string("REMOVED\n");
		writeMessageList(removed_list);
		m_output += std::string("ADDED\n");
		writeMessageList(added_list);
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

