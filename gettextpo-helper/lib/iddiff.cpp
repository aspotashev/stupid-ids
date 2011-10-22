
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <set>

#include <git2.h>
#include <gettextpo-helper/gettextpo-helper.h>
#include <gettextpo-helper/translationcontent.h>
#include <gettextpo-helper/stupids-client.h>
#include <gettextpo-helper/translation-collector.h>
#include <gettextpo-helper/message.h>
#include <gettextpo-helper/iddiff.h>


IddiffMessage::IddiffMessage():
	MessageTranslationBase()
{
}

IddiffMessage::IddiffMessage(const IddiffMessage &msg)
{
	m_fuzzy = msg.isFuzzy();

	m_numPlurals = msg.numPlurals();
	for (int i = 0; i < m_numPlurals; i ++)
		m_msgstr[i] = xstrdup(msg.msgstr(i));
}

IddiffMessage::IddiffMessage(const Message &msg)
{
    m_fuzzy = msg.isFuzzy();

    m_numPlurals = msg.numPlurals();
    for (int i = 0; i < m_numPlurals; i ++)
        m_msgstr[i] = xstrdup(msg.msgstr(i));
}

IddiffMessage::IddiffMessage(po_message_t message):
	MessageTranslationBase(message)
{
}

IddiffMessage::~IddiffMessage()
{
	// TODO: free memory
}

void IddiffMessage::addMsgstr(const char *str)
{
	assert(m_numPlurals < MAX_PLURAL_FORMS);
	m_numPlurals ++;

	m_msgstr[m_numPlurals - 1] = xstrdup(str);
}

void IddiffMessage::setFuzzy(bool fuzzy)
{
	m_fuzzy = fuzzy;
}

/**
 * Fuzzy flag state will not be copied.
 */
// TODO: how to replace this?
void IddiffMessage::copyTranslationsToMessage(Message *message) const
{
	assert(numPlurals() == message->numPlurals());

	for (int i = 0; i < numPlurals(); i ++)
		message->editMsgstr(i, msgstr(i));
}

bool IddiffMessage::isTranslated() const
{
	if (isFuzzy())
		return false;

	for (int i = 0; i < numPlurals(); i ++)
		if (strlen(msgstr(i)) > 0)
			return true;

	return false;
}

//----------------------------------------------

IddiffChange::IddiffChange()
{
	m_reviewComment = NULL;
}

void IddiffChange::clearReviewComment()
{
	m_reviewComment = NULL;
}

bool IddiffChange::empty() const
{
	return emptyReview() && emptyAdded() && emptyRemoved();
}

bool IddiffChange::emptyRemoved() const
{
	return m_removedItems.empty();
}

bool IddiffChange::emptyAdded() const
{
	return m_addedItems.empty();
}

bool IddiffChange::emptyReview() const
{
	return m_reviewComment == NULL;
}

// static
void IddiffChange::eraseItem(std::vector<IddiffMessage *> &list, const IddiffMessage *item)
{
	for (std::vector<IddiffMessage *>::iterator iter = list.begin(); iter != list.end(); iter ++)
		if (*iter == item) // comparison by pointer, not by translations!
		{
			list.erase(iter);
			return;
		}

	assert(0); // not found!
}

void IddiffChange::eraseRemoved(const IddiffMessage *item)
{
	eraseItem(m_removedItems, item);
}

void IddiffChange::eraseAdded(const IddiffMessage *item)
{
	eraseItem(m_addedItems, item);
}

//----------------------------------------------

/**
 * \brief Constructs an empty iddiff
 */
Iddiffer::Iddiffer():
	m_output(std::ostringstream::out), m_minimizedIds(false)
{
}

Iddiffer::~Iddiffer()
{
	std::vector<std::pair<int, IddiffMessage *> > removed_list = getRemovedVector();
	for (size_t i = 0; i < removed_list.size(); i ++)
		delete removed_list[i].second;

	std::vector<std::pair<int, IddiffMessage *> > added_list = getAddedVector();
	for (size_t i = 0; i < added_list.size(); i ++)
		delete added_list[i].second;

	std::vector<std::pair<int, IddiffMessage *> > review_list = getReviewVector();
	for (size_t i = 0; i < review_list.size(); i ++)
		delete review_list[i].second;
}

void Iddiffer::writeMessageList(std::vector<std::pair<int, IddiffMessage *> > list)
{
	for (size_t i = 0; i < list.size(); i ++)
		m_output << list[i].first << " " << list[i].second->formatPoMessage() << "\n";
}

void Iddiffer::clearIddiff()
{
	m_items.clear();
}

void Iddiffer::clearReviewComments()
{
	for (std::map<int, IddiffChange>::iterator it = m_items.begin(); it != m_items.end(); it ++)
		it->second.clearReviewComment();
}

void Iddiffer::clearReviewComment(int msg_id)
{
	m_items[msg_id].clearReviewComment();
}

// This function fills m_addedItems.
// m_removedItems and m_reviewComments will be cleared.
void Iddiffer::diffAgainstEmpty(TranslationContent *content_b)
{
	clearIddiff();

	const git_oid *tp_hash = content_b->calculateTpHash();
	int first_id = stupidsClient.getFirstId(tp_hash);
	assert(first_id > 0);


	// TODO: function for this
	m_date = content_b->date();
	m_author = content_b->author();

	// compare pairs of messages in 2 .po files
	po_file_t file_b = content_b->poFileRead();
	po_message_iterator_t iterator_b = po_message_iterator(file_b, "messages");
	// skipping headers
	po_message_t message_b = po_next_message(iterator_b);

	// TODO: use data from TranslationContent::readMessages
	for (int index = 0;
		(message_b = po_next_message(iterator_b)) &&
		!po_message_is_obsolete(message_b);
		index ++)
	{
		// Messages can be:
		//     "" -- untranslated (does not matter fuzzy or not, so f"" is illegal)
		//     "abc" -- translated
		//     f"abc" -- fuzzy

		// Types of possible changes:
		//     ""     -> ""     : -
		//     ""     -> "abc"  : ADDED
		//     ""     -> f"abc" : - (fuzzy messages are "weak", you should write in comments instead what you are not sure in)

		// Adding to "ADDED" if "B" is translated
		if (!po_message_is_untranslated(message_b) && !po_message_is_fuzzy(message_b))
			insertAdded(first_id + index, new IddiffMessage(message_b));
	}

	// free memory
	po_message_iterator_free(iterator_b);
	po_file_free(file_b);
}

// This function fills m_removedItems and m_addedItems
// m_reviewComments will be cleared.
void Iddiffer::diffFiles(TranslationContent *content_a, TranslationContent *content_b)
{
	clearIddiff();

	// .po files should be derived from the same .pot
	const git_oid *tp_hash = content_a->calculateTpHash();
	assert(git_oid_cmp(tp_hash, content_b->calculateTpHash()) == 0);

	// first_id is the same for 2 files
	int first_id = stupidsClient.getFirstId(tp_hash);
	assert(first_id > 0);


	// TODO: function for this
	m_date = content_b->date();
	m_author = content_b->author();

	// compare pairs of messages in 2 .po files
// 	po_file_t file_a = content_a->poFileRead();
// 	po_file_t file_b = content_b->poFileRead();
//
// 	po_message_iterator_t iterator_a =
// 		po_message_iterator(file_a, "messages");
// 	po_message_iterator_t iterator_b =
// 		po_message_iterator(file_b, "messages");
// 	// skipping headers
// 	po_message_t message_a = po_next_message(iterator_a);
// 	po_message_t message_b = po_next_message(iterator_b);
//
// 	// TODO: use data from TranslationContent::readMessages
// 	for (int index = 0;
// 		(message_a = po_next_message(iterator_a)) &&
// 		(message_b = po_next_message(iterator_b)) &&
// 		!po_message_is_obsolete(message_a) &&
// 		!po_message_is_obsolete(message_b);
// 		index ++)
// 	{

    std::vector<MessageGroup *> messages_a = content_a->readMessages();
    std::vector<MessageGroup *> messages_b = content_b->readMessages();
    for (size_t index = 0; index < messages_a.size(); index ++)
    {
        Message *message_a = messages_a[index]->message(0);
        Message *message_b = messages_b[index]->message(0);

// 		if (strcmp(po_message_comments(message_a), po_message_comments(message_b)))
// 		{
// 			fprintf(stderr, "Changes in comments will be ignored!\n");
// 			fprintf(stderr, "<<<<<\n");
// 			fprintf(stderr, "%s", po_message_comments(message_a)); // "\n" should be included in comments
// 			fprintf(stderr, "=====\n");
// 			fprintf(stderr, "%s", po_message_comments(message_b)); // "\n" should be included in comments
// 			fprintf(stderr, ">>>>>\n");
// 		}


		// Messages can be:
		//     "" -- untranslated (does not matter fuzzy or not, so f"" is illegal)
		//     "abc" -- translated
		//     f"abc" -- fuzzy

		// bit 0 -- empty A
		// bit 1 -- empty B
		// bit 2 -- A == B (msgstr)
		// bit 3 -- fuzzy A
		// bit 4 -- fuzzy B
		#define FORMAT_IDDIFF_BITARRAY(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13) { \
			p1,  /* 00000    "abc" ->  "def"                                           */ \
			p2,  /* 00001    ""    ->  "abc"                                           */ \
			p3,  /* 00010    "abc" ->  ""                                              */ \
			-1,  /* 00011   (both empty, but not equal)                                */ \
			p4,  /* 00100    "abc" ->  "abc"                                           */ \
			-1,  /* 00101   (empty cannot be equal to non-empty)                       */ \
			-1,  /* 00110   (empty cannot be equal to non-empty)                       */ \
			p5,  /* 00111       "" ->  ""                                              */ \
			p6,  /* 01000   f"abc" ->  "def"                                           */ \
			-1,  /* 01001   (empty+fuzzy A)                                            */ \
			p7,  /* 01010   f"abc" ->  ""                                              */ \
			-1,  /* 01011   (empty+fuzzy A, both empty but not equal)                  */ \
			p8,  /* 01100   f"abc" ->  "abc"                                           */ \
			-1,  /* 01101   (empty+fuzzy A, empty cannot be equal to non-empty)        */ \
			-1,  /* 01110   (empty cannot be equal to non-empty)                       */ \
			-1,  /* 01111   (empty+fuzzy A)                                            */ \
			p9,  /* 10000    "abc" -> f"def"                                           */ \
			p10, /* 10001       "" -> f"abc"                                           */ \
			-1,  /* 10010   (empty+fuzzy B)                                            */ \
			-1,  /* 10011   (empty+fuzzy B, both empty, but not equal)                 */ \
			p11, /* 10100    "abc" -> f"abc"                                           */ \
			-1,  /* 10101   (empty cannot be equal to non-empty)                       */ \
			-1,  /* 10110   (empty+fuzzy B, empty cannot be equal to non-empty)        */ \
			-1,  /* 10111   (empty+fuzzy B)                                            */ \
			p12, /* 11000   f"abc" -> f"def"                                           */ \
			-1,  /* 11001   (empty+fuzzy A)                                            */ \
			-1,  /* 11010   (empty+fuzzy B)                                            */ \
			-1,  /* 11011   (empty+fuzzy A, empty+fuzzy B, both empty, but not equal)  */ \
			p13, /* 11100   f"abc" -> f"abc"                                           */ \
			-1,  /* 11101   (empty+fuzzy A, empty cannot be equal to non-empty)        */ \
			-1,  /* 11110   (empty+fuzzy B, empty cannot be equal to non-empty)        */ \
			-1,  /* 11111   (empty+fuzzy A, empty+fuzzy B)                             */ \
		}

		// Types of possible changes:
		// 00000    "abc"  -> "def"  : REMOVED, ADDED
		// 00001    ""     -> "abc"  : ADDED
		// 00010    "abc"  -> ""     : REMOVED
		// 00100    "abc"  -> "abc"  : -
		// 00111    ""     -> ""     : -
		// 01000    f"abc" -> "def"  : REMOVED, ADDED
		// 01010    f"abc" -> ""     : REMOVED (removing fuzzy messages is OK)
		// 01100    f"abc" -> "abc"  : ADDED
		// 10000    "abc"  -> f"def" : REMOVED
		// 10001    ""     -> f"abc" : - (fuzzy messages are "weak", you should write in comments instead what you are not sure in)
		// 10100    "abc"  -> f"abc" : REMOVED
		// 11000    f"abc" -> f"def" : - (fuzzy messages are "weak", you should write in comments instead what you are not sure in)
		// 11100    f"abc" -> f"abc" : -
		const int   needAdded[] = FORMAT_IDDIFF_BITARRAY(1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0);
		const int needRemoved[] = FORMAT_IDDIFF_BITARRAY(1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0);

		int indexCode = 0;
// 		indexCode |= po_message_is_untranslated(message_a) ? 1 : 0;
// 		indexCode |= po_message_is_untranslated(message_b) ? 2 : 0;
// 		indexCode |= compare_po_message_msgstr(message_a, message_b) ? 0 : 4;
// 		indexCode |= po_message_is_fuzzy(message_a) ? 8 : 0;
// 		indexCode |= po_message_is_fuzzy(message_b) ? 16 : 0;
        indexCode |= message_a->isUntranslated() ? 1 : 0;
        indexCode |= message_b->isUntranslated() ? 2 : 0;
        indexCode |= message_a->equalMsgstr(message_b) ? 4 : 0;
        indexCode |= message_a->isFuzzy() ? 8 : 0;
        indexCode |= message_b->isFuzzy() ? 16 : 0;

		assert(indexCode >= 0 && indexCode < 32);
		assert(needRemoved[indexCode] != -1);
		assert(needAdded[indexCode] != -1);

		if (needRemoved[indexCode])
			insertRemoved(first_id + index, new IddiffMessage(*message_a));
		if (needAdded[indexCode])
			insertAdded(first_id + index, new IddiffMessage(*message_b));
	}

	// free memory
// 	po_message_iterator_free(iterator_a);
// 	po_message_iterator_free(iterator_b);
// 	po_file_free(file_a);
// 	po_file_free(file_b);
}

// http://www.infosoftcom.ru/article/realizatsiya-funktsii-split-string
std::vector<std::string> split_string(const std::string &str, const std::string &sep)
{
	size_t str_len = (int)str.length();
	size_t sep_len = (int)sep.length();
	assert(sep_len > 0);


	std::vector<std::string> res;

	int j = 0;
	int i = str.find(sep, j);
	while (i != -1)
	{
		if (i > j && i <= str_len)
			res.push_back(str.substr(j, i - j));
		j = i + sep_len;
		i = str.find(sep, j);
	}

	int l = str_len - 1;
	if (str.substr(j, l - j + 1).length() > 0)
		res.push_back(str.substr(j, l - j + 1));

	return res;
}

std::string join_strings(const std::vector<std::string> &str, const std::string &sep)
{
	std::string res;
	for (size_t i = 0; i < str.size(); i ++)
	{
		if (i > 0)
			res += sep;
		res += str[i];
	}

	return res;
}

// TODO: write and use function split_string()
std::set<std::string> set_of_lines(const char *input)
{
	std::set<std::string> res;

	std::string input_str(input);
	std::stringstream ss(input_str);
	std::string item;
	while (std::getline(ss, item))
		res.insert(item);

	return res;
}

template <typename T>
std::vector<T> set_difference(const std::set<T> &a, const std::set<T> &b)
{
	std::vector<T> res(a.size());
	typename std::vector<T>::iterator it = set_difference(
		a.begin(), a.end(), b.begin(), b.begin(), res.begin());
	res.resize(it - res.begin());

	return res;
}

void Iddiffer::diffTrCommentsAgainstEmpty(TranslationContent *content_b)
{
	clearIddiff();

	const git_oid *tp_hash = content_b->calculateTpHash();
	int first_id = stupidsClient.getFirstId(tp_hash);
	assert(first_id > 0);


	// TODO: function for this
	m_date = content_b->date();
	m_author = content_b->author();

	// compare pairs of messages in 2 .po files
	po_file_t file_b = content_b->poFileRead();
	po_message_iterator_t iterator_b = po_message_iterator(file_b, "messages");
	// skipping headers
	po_message_t message_b = po_next_message(iterator_b);

	// TODO: use data from TranslationContent::readMessages
	for (int index = 0;
		(message_b = po_next_message(iterator_b)) &&
		!po_message_is_obsolete(message_b);
		index ++)
	{
		std::set<std::string> comm_a; // empty
		std::set<std::string> comm_b = set_of_lines(po_message_comments(message_b));

		std::vector<std::string> added = set_difference(comm_b, comm_a);

		IddiffMessage *diff_message = new IddiffMessage();
		for (size_t i = 0; i < added.size(); i ++)
			diff_message->addMsgstr(added[i].c_str());
		if (added.size() > 0)
			insertAdded(first_id + index, diff_message);
	}

	// free memory
	po_message_iterator_free(iterator_b);
	po_file_free(file_b);
}

void Iddiffer::diffTrCommentsFiles(TranslationContent *content_a, TranslationContent *content_b)
{
	clearIddiff();

	// .po files should be derived from the same .pot
	const git_oid *tp_hash = content_a->calculateTpHash();
	assert(git_oid_cmp(tp_hash, content_b->calculateTpHash()) == 0);

	// first_id is the same for 2 files
	int first_id = stupidsClient.getFirstId(tp_hash);
	assert(first_id > 0);


	// TODO: function for this
	m_date = content_b->date();
	m_author = content_b->author();

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

	// TODO: use data from TranslationContent::readMessages
	for (int index = 0;
		(message_a = po_next_message(iterator_a)) &&
		(message_b = po_next_message(iterator_b)) &&
		!po_message_is_obsolete(message_a) &&
		!po_message_is_obsolete(message_b);
		index ++)
	{
		if (strcmp(po_message_comments(message_a), po_message_comments(message_b)) == 0)
			continue;

		std::set<std::string> comm_a = set_of_lines(po_message_comments(message_a));
		std::set<std::string> comm_b = set_of_lines(po_message_comments(message_b));

		std::vector<std::string> added = set_difference(comm_b, comm_a);

		IddiffMessage *diff_message = new IddiffMessage();
		for (size_t i = 0; i < added.size(); i ++)
			diff_message->addMsgstr(added[i].c_str());
		if (added.size() > 0)
			insertAdded(first_id + index, diff_message);
	}

	// free memory
	po_message_iterator_free(iterator_a);
	po_message_iterator_free(iterator_b);
	po_file_free(file_a);
	po_file_free(file_b);
}

const FileDateTime &Iddiffer::date() const
{
	return m_date;
}

std::string Iddiffer::dateString() const
{
	return m_date.dateString();
}

std::string Iddiffer::generateIddiffText()
{
	m_output.str("");
	std::vector<std::pair<int, IddiffMessage *> > removed_list = getRemovedVector();
	std::vector<std::pair<int, IddiffMessage *> > added_list = getAddedVector();
	std::vector<std::pair<int, IddiffMessage *> > review_list = getReviewVector();
	if (removed_list.size() > 0 || added_list.size() > 0 || review_list.size() > 0)
	{
		m_output << "Subject: " << m_subject << "\n";
		m_output << "Author: " << m_author << "\n";
		m_output << "Date: " << dateString() << "\n";
		m_output << "\n";

		if (removed_list.size() > 0)
		{
			m_output << "REMOVED\n";
			writeMessageList(removed_list);
		}

		if (added_list.size() > 0)
		{
			m_output << "ADDED\n";
			writeMessageList(added_list);
		}

		if (review_list.size() > 0)
		{
			m_output << "REVIEW\n";
			writeMessageList(review_list);
		}
	}

	return m_output.str();
}

bool Iddiffer::loadIddiff(const char *filename)
{
	// TODO: function for reading the whole file
	FILE *f = fopen(filename, "r");
	if (!f)
		return false; // file does not exist

	fseek(f, 0, SEEK_END);
	int file_size = (int)ftell(f);
	rewind(f);

	if (file_size == 0)
	{
		fclose(f);
		return false; // file is empty
	}

	char *buffer = new char[file_size + 1];
	assert(fread(buffer, 1, file_size, f) == file_size);
	fclose(f);
	buffer[file_size] = '\0';

	std::vector<char *> lines;

	char *cur_line = buffer;
	char *line_break = buffer; // something that is not NULL

	bool has_subject = false;
	bool has_author = false;
	m_date.clear();
	while (line_break)
	{
		line_break = strchr(cur_line, '\n');
		if (line_break == NULL) // last line does not end with "\n"
		{
			break;
		}
		else if (line_break - buffer == file_size - 1) // last line ends with "\n"
		{
			*line_break = '\0';
			line_break = NULL;
		}

		if (line_break)
			*line_break = '\0';

		lines.push_back(cur_line);
		cur_line = line_break + 1;
	}

	// Reading header
	size_t index = 0;
	for (index; index < lines.size(); index ++)
	{
		char *line = lines[index];

		if (strlen(line) == 0)
		{
			// the header ends
			index ++;
			break;
		}

		char *colon = strchr(line, ':');
		assert(colon);

		colon[0] = '\0'; // truncate at the end of header item key
		assert(colon[1] == ' '); // space after colon

		colon += 2; // move to the header item value

		if (!strcmp(line, "Subject"))
		{
			assert(!has_subject); // catch duplicate "Subject:" fields
			has_subject = true;

			m_subject = std::string(colon);
		}
		else if (!strcmp(line, "Author"))
		{
			assert(!has_author); // catch duplicate "Author:" fields
			has_author = true;

			m_author = std::string(colon);
		}
		else if (!strcmp(line, "Date"))
		{
			assert(m_date.isNull()); // catch duplicate "Date:" fields

			std::string date_string = std::string(colon);
			m_date.fromString(date_string.c_str());
		}
		else
		{
			printf("Unknown header field: [%s]\n", line);
			printf("File: [%s]\n", filename);
			assert(0);
		}
	}
	// Check that all fields are present (is it really necessary?)
	assert(has_subject);
	assert(has_author);
	if (m_date.isNull())
		fprintf(stderr, "Warning: the .iddiff file does not have a \"Date:\" field in the header\n");


	enum
	{
		SECTION_UNKNOWN,
		SECTION_REMOVED,
		SECTION_ADDED,
		SECTION_REVIEW,
	} current_section = SECTION_UNKNOWN;

	bool has_removed = false;
	bool has_added = false;
	bool has_review = false;
	for (index; index < lines.size(); index ++)
	{
		char *line = lines[index];

		if (!strcmp(line, "REMOVED"))
		{
			assert(!has_removed);
			has_removed = true;

			current_section = SECTION_REMOVED;
		}
		else if (!strcmp(line, "ADDED"))
		{
			assert(!has_added);
			has_added = true;

			current_section = SECTION_ADDED;
		}
		else if (!strcmp(line, "REVIEW"))
		{
			assert(!has_review);
			has_review = true;

			current_section = SECTION_REVIEW;
		}
		// Now we are processing a line inside a section
		else if (current_section == SECTION_REMOVED)
			insertRemoved(loadMessageListEntry(line));
		else if (current_section == SECTION_ADDED)
			insertAdded(loadMessageListEntry(line));
		else if (current_section == SECTION_REVIEW)
			insertReview(loadMessageListEntry(line));
		else // invalid value of "current_section"
		{
			// Unknown iddiff section
			assert(0);
		}
	}

	// Check that the .iddiff was not useless
	assert(has_removed || has_added || has_review);


	delete [] buffer;
	return true; // OK
}

void Iddiffer::writeToFile(const char *filename)
{
	std::string text = generateIddiffText();
	if (!text.empty())
	{
		std::ofstream out(filename, std::ios::out | std::ios::trunc);
		out << text;
		out.close();
	}
}

/**
 * Allocates an object of class IddiffMessage
 */
std::pair<int, IddiffMessage *> Iddiffer::loadMessageListEntry(const char *line)
{
	const char *space = strchr(line, ' ');
	assert(space);
	for (const char *cur = line; cur < space; cur ++)
		assert(isdigit(*cur));

	int msg_id = atoi(line);
	line = space + 1;

//	printf("%d <<<%s>>>\n", msg_id, line);

	IddiffMessage *msg = new IddiffMessage();
	char *msgstr_buf = new char[strlen(line) + 1];
	while (*line != '\0')
	{
		if (*line == 'f')
		{
			msg->setFuzzy(true);
			line ++;
		}

		assert(*line == '\"');
		line ++;

		int msgstr_i = 0;
		while (*line != '\0')
		{
			if (*line == '\"')
				break;
			else if (line[0] == '\\' && line[1] == '\"')
			{
				msgstr_buf[msgstr_i] = '\"';
				line ++;
			}
			else if (line[0] == '\\' && line[1] == 'n')
			{
				msgstr_buf[msgstr_i] = '\n';
				line ++;
			}
			else if (line[0] == '\\' && line[1] == 't')
			{
				msgstr_buf[msgstr_i] = '\t';
				line ++;
			}
			else if (line[0] == '\\' && line[1] == '\\')
			{
				msgstr_buf[msgstr_i] = '\\';
				line ++;
			}
			else
			{
				msgstr_buf[msgstr_i] = *line;
			}

			line ++;
			msgstr_i ++;
		}
		msgstr_buf[msgstr_i] = '\0';
		msg->addMsgstr(msgstr_buf);
//		printf("[[[%s]]]\n", msgstr_buf);

		assert(*line == '\"');
		line ++;
		if (*line == ' ')
			line ++;
		else
			assert(*line == '\0');
	}

	delete [] msgstr_buf;
	return std::make_pair<int, IddiffMessage *>(msg_id, msg);
}

template <typename T>
void sort_uniq(std::vector<T> &v)
{
	sort(v.begin(), v.end());
	v.resize(unique(v.begin(), v.end()) - v.begin());
}

std::vector<int> Iddiffer::involvedIds()
{
	std::vector<int> res;
	// TODO: macro for walking through an std::map
	for (std::map<int, IddiffChange>::iterator iter = m_items.begin(); iter != m_items.end(); iter ++)
		res.push_back(iter->first);

	sort_uniq(res);
	return res;
}

void Iddiffer::cleanupMsgIdData(int msg_id)
{
	std::map<int, IddiffChange>::iterator it = m_items.find(msg_id);
	if (it != m_items.end() && it->second.empty())
		m_items.erase(it);
}

void Iddiffer::substituteMsgId(int old_id, int new_id)
{
	if (old_id == new_id)
		return;

	cleanupMsgIdData(old_id);
	cleanupMsgIdData(new_id);

	// Check that IDs won't collide
	if (m_items.find(new_id) != m_items.end())
	{
		// TODO: if there is no real conflict, this should not fail
		// (e.g. when the same string was changed in the same way via two different msg_IDs,
		// this often happens if you enable parallel branches in Lokalize)
		printf("new_id = %d, old_id = %d\n", new_id, old_id);
		assert(0);
	}

	if (m_items.find(old_id) != m_items.end())
	{
		m_items[new_id] = m_items[old_id]; // copying object of class IddiffChange
		m_items.erase(old_id);
	}
}

void Iddiffer::minimizeIds()
{
	std::vector<int> msg_ids_arr = involvedIds();

	// Request minimized IDs from server
	std::vector<int> min_ids_arr = stupidsClient.getMinIds(msg_ids_arr);
	assert(msg_ids_arr.size() == min_ids_arr.size());

	for (size_t i = 0; i < msg_ids_arr.size(); i ++)
		substituteMsgId(msg_ids_arr[i], min_ids_arr[i]);

	m_minimizedIds = true;
}

// If the return value is true, we can put the new translation and drop the old one (when patching)
bool Iddiffer::canDropMessage(const Message *message, int min_id)
{
	std::vector<IddiffMessage *> removed = findRemoved(min_id);

	if (message->isFuzzy() || message->isUntranslated())
		return true;

	for (size_t i = 0; i < removed.size(); i ++) // TODO: implement this through findRemoved (when Message and IddiffMessage will be merged)
		if (removed[i]->equalMsgstr(message))
			return true;
	return false;
}

void Iddiffer::applyToMessage(MessageGroup *messageGroup, int min_id)
{
	assert(messageGroup->size() == 1);
	Message *message = messageGroup->message(0);
	IddiffMessage *added = findAddedSingle(min_id);

	if (canDropMessage(message, min_id))
	{
		if (added) // change translation
		{
			added->copyTranslationsToMessage(message);
			message->editFuzzy(false);
		}
		else // fuzzy old translation
		{
			//printf("Marking as FUZZY!!!\n");
			message->editFuzzy(true);
		}
	}
	else // !canDropMessage
	{
		// We cannot just add the new translation without
		// "blacklisting" the existing one.
		if (added && !added->equalTranslations(message))
		{
			std::cerr << "You have a conflict:" << std::endl <<
				" * Someone has already translated this message as:" << std::endl <<
				message->formatPoMessage() << std::endl <<
				" * And you are suggesting this:" << std::endl <<
				added->formatPoMessage() << std::endl;
			assert(0);
		}
	}
}

void Iddiffer::applyToMessageComments(MessageGroup *messageGroup, int min_id)
{
	assert(messageGroup->size() == 1);
	Message *message = messageGroup->message(0);

	std::set<std::string> comments = set_of_lines(message->msgcomments());

	IddiffMessage *removed = findRemovedSingle(min_id);
	if (removed)
		for (int i = 0; i < removed->numPlurals(); i ++)
			comments.erase(std::string(removed->msgstr(i)));

	IddiffMessage *added = findAddedSingle(min_id);
	if (added)
		for (int i = 0; i < added->numPlurals(); i ++)
			comments.insert(std::string(added->msgstr(i)));


	std::vector<std::string> comments_vec;
	for (std::set<std::string>::iterator it = comments.begin();
	     it != comments.end(); it ++)
	     comments_vec.push_back(*it);
	// TODO: make join_strings accept any container, including std::set (or iterator?)
	std::string joined_comments = join_strings(comments_vec, std::string("\n"));

	message->editMsgcomments(joined_comments.c_str());
}

// TODO: remove this function!
// Applies the iddiff to the given TranslationContent and writes changes to file
void Iddiffer::applyToContent(TranslationContent *content)
{
	std::vector<MessageGroup *> messages = content->readMessages();
	std::vector<int> min_ids = content->getMinIds();
	std::vector<int> involved_ids = involvedIds();

	for (size_t i = 0; i < messages.size(); i ++)
		if (binary_search(involved_ids.begin(), involved_ids.end(), min_ids[i]))
			applyToMessage(messages[i], min_ids[i]);
}

// TODO: Warn about messages that are involved in the Iddiff, but were not found in any of the .po files
void Iddiffer::applyIddiff(StupIdTranslationCollector *collector, bool applyComments)
{
	// Check that involvedIds() will return minimized IDs
	assert(m_minimizedIds);

	std::map<int, std::vector<MessageGroup *> > messages;
	std::vector<TranslationContent *> contents;
	collector->getMessagesByIds(messages, contents, involvedIds());

	std::vector<int> involved_ids = involvedIds();
	for (size_t i = 0; i < involved_ids.size(); i ++)
	{
		int min_id = involved_ids[i];
		std::vector<MessageGroup *> messageGroups = messages[min_id];
		for (size_t j = 0; j < messageGroups.size(); j ++)
		{
			if (applyComments)
				applyToMessageComments(messageGroups[j], min_id);
			else
				applyToMessage(messageGroups[j], min_id);
		}

		if (messageGroups.size() == 0)
			fprintf(stderr,
				"Message has not been found nowhere by its ID (%d):\t%s\n",
				min_id, findAddedSingle(min_id)->formatPoMessage().c_str());
	}

	printf("involved contents: %d\n", (int)contents.size());
	for (size_t i = 0; i < contents.size(); i ++)
	{
		TranslationContent *content = contents[i];
		printf("Writing %s\n", content->displayFilename());
		content->writeToFile(content->displayFilename(), true); // TODO: StupIdTranslationCollector::writeChanges() for writing all changes to .po files
	}
}

void Iddiffer::applyIddiff(StupIdTranslationCollector *collector)
{
	applyIddiff(collector, false);
}

void Iddiffer::applyIddiffComments(StupIdTranslationCollector *collector)
{
	applyIddiff(collector, true);
}

/**
 * \brief Add translation version to the "REMOVED" section ensuring that it does not conflict with existing Iddiff entries.
 *
 * Takes ownership of "item".
 */
void Iddiffer::insertRemoved(int msg_id, IddiffMessage *item)
{
	if (findRemoved(msg_id, item) != NULL) // duplicate in "REMOVED"
		return;

	assert(findAdded(msg_id, item) == NULL); // conflict: trying to "REMOVE" a translation already existing in "ADDED"

	m_items[msg_id].m_removedItems.push_back(item);
}

/**
 * \brief Add translation version to the "ADDED" section ensuring that it does not conflict with existing Iddiff entries.
 *
 * Takes ownership of "item".
 */
void Iddiffer::insertAdded(int msg_id, IddiffMessage *item)
{
	if (findRemoved(msg_id, item) != NULL) // conflict: trying to "ADD" a translation already existing in "REMOVED"
	{
		fprintf(stderr,
			"Conflict: removing previously added translation\n"
			"\tmsg_id = %d\n"
            "\told message translation: %s\n"
			"\tnew message translation: %s\n",
            msg_id,
            findRemoved(msg_id, item)->formatPoMessage().c_str(),
            item->formatPoMessage().c_str());
		assert(0);
	}

	std::vector<IddiffMessage *> added_this_id = findAdded(msg_id);
	for (size_t i = 0; i < added_this_id.size(); i ++)
	{
		if (added_this_id[i]->equalMsgstr(item))
			return; // duplicate in "ADDED"
		else
			assert(0); // conflict: two different translations in "ADDED"
	}

	m_items[msg_id].m_addedItems.push_back(item);
}

/**
 * \brief Add message to the "REVIEW" section ensuring that there was no "REVIEW" entry for the given msg_id.
 *
 * Takes ownership of "item".
 */
void Iddiffer::insertReview(int msg_id, IddiffMessage *item)
{
	assert(m_items[msg_id].emptyReview());
	assert(item->isTranslated()); // text should not be empty

	m_items[msg_id].m_reviewComment = item;
}

/**
 * Takes ownership of "item.second".
 */
void Iddiffer::insertRemoved(std::pair<int, IddiffMessage *> item)
{
	insertRemoved(item.first, item.second);
}

/**
 * Takes ownership of "item.second".
 */
void Iddiffer::insertAdded(std::pair<int, IddiffMessage *> item)
{
	insertAdded(item.first, item.second);
}

/**
 * Takes ownership of "item.second".
 */
void Iddiffer::insertReview(std::pair<int, IddiffMessage *> item)
{
	insertReview(item.first, item.second);
}

/**
 * Does not take ownership of "item.second".
 */
void Iddiffer::insertRemovedClone(std::pair<int, IddiffMessage *> item)
{
	insertRemoved(item.first, new IddiffMessage(*item.second));
}

/**
 * Does not take ownership of "item.second".
 */
void Iddiffer::insertAddedClone(std::pair<int, IddiffMessage *> item)
{
	insertAdded(item.first, new IddiffMessage(*item.second));
}

/**
 * Does not take ownership of "item.second".
 */
void Iddiffer::insertReviewClone(std::pair<int, IddiffMessage *> item)
{
	insertReview(item.first, new IddiffMessage(*item.second));
}

std::vector<std::pair<int, IddiffMessage *> > Iddiffer::getRemovedVector()
{
	std::vector<std::pair<int, IddiffMessage *> > res;
	for (std::map<int, IddiffChange>::iterator iter = m_items.begin();
		iter != m_items.end(); iter ++)
	{
		std::vector<IddiffMessage *> id_items = iter->second.m_removedItems;
		for (size_t i = 0; i < id_items.size(); i ++)
			res.push_back(std::make_pair<int, IddiffMessage *>(iter->first, id_items[i]));
	}

	return res;
}

std::vector<std::pair<int, IddiffMessage *> > Iddiffer::getAddedVector()
{
	std::vector<std::pair<int, IddiffMessage *> > res;
	for (std::map<int, IddiffChange>::iterator iter = m_items.begin();
		iter != m_items.end(); iter ++)
	{
		std::vector<IddiffMessage *> id_items = iter->second.m_addedItems;
		for (size_t i = 0; i < id_items.size(); i ++)
			res.push_back(std::make_pair<int, IddiffMessage *>(iter->first, id_items[i]));
	}

	return res;
}

std::vector<std::pair<int, IddiffMessage *> > Iddiffer::getReviewVector()
{
	std::vector<std::pair<int, IddiffMessage *> > res;
	for (std::map<int, IddiffChange>::iterator iter = m_items.begin();
		iter != m_items.end(); iter ++)
	{
		if (iter->second.m_reviewComment)
			res.push_back(std::make_pair<int, IddiffMessage *>(iter->first, iter->second.m_reviewComment));
	}

	return res;
}

void Iddiffer::mergeHeaders(Iddiffer *diff)
{
	// Keep the most recent date/time
	if (m_date.isNull() || diff->date() > m_date)
		m_date = diff->date();

	std::vector<std::string> authors = split_string(m_author, std::string(", "));
	std::vector<std::string> added_authors = split_string(diff->m_author, std::string(", "));
	authors.insert(authors.end(), added_authors.begin(), added_authors.end());
	sort_uniq(authors);

	m_author = join_strings(authors, std::string(", "));
}

// TODO: Iddiffer::mergeNokeep that removes all items from the given Iddiff (and therefore it does not need to clone the IddiffMessage objects)
void Iddiffer::merge(Iddiffer *diff)
{
	std::vector<std::pair<int, IddiffMessage *> > other_removed = diff->getRemovedVector();
	std::vector<std::pair<int, IddiffMessage *> > other_added = diff->getAddedVector();
	std::vector<std::pair<int, IddiffMessage *> > other_review = diff->getReviewVector();

	for (size_t i = 0; i < other_removed.size(); i ++)
		insertRemovedClone(other_removed[i]); // this also clones the IddiffMessage object
	for (size_t i = 0; i < other_added.size(); i ++)
		insertAddedClone(other_added[i]); // this also clones the IddiffMessage object
	for (size_t i = 0; i < other_review.size(); i ++)
		insertReviewClone(other_review[i]); // this also clones the IddiffMessage object

	mergeHeaders(diff);
}

void Iddiffer::mergeTrComments(Iddiffer *diff)
{
	assert(diff->getReviewVector().empty());

	std::vector<std::pair<int, IddiffMessage *> > other_removed = diff->getRemovedVector();
	std::vector<std::pair<int, IddiffMessage *> > other_added = diff->getAddedVector();

	assert(other_removed.empty()); // TODO: implement later
//	for (size_t i = 0; i < other_removed.size(); i ++)
//		insertRemovedClone(other_removed[i]); // this also clones the IddiffMessage object

	for (size_t i = 0; i < other_added.size(); i ++)
	{
		int msg_id = other_added[i].first;
		IddiffMessage *message = other_added[i].second;

		IddiffMessage *prevMessage = findAddedSingle(msg_id);
		if (prevMessage)
		{
			std::vector<std::string> comments;
			for (int i = 0; i < message->numPlurals(); i ++)
				comments.push_back(std::string(message->msgstr(i)));
			for (int i = 0; i < prevMessage->numPlurals(); i ++)
				comments.push_back(std::string(prevMessage->msgstr(i)));

			sort_uniq(comments);

			IddiffMessage *new_message = new IddiffMessage();
			for (size_t i = 0; i < comments.size(); i ++)
				new_message->addMsgstr(comments[i].c_str());

			eraseAdded(msg_id, prevMessage);
			insertAddedClone(std::make_pair(msg_id, new_message));
		}
		else
		{
			insertAddedClone(other_added[i]);
		}
	}

	mergeHeaders(diff);
}

std::vector<IddiffMessage *> Iddiffer::findRemoved(int msg_id)
{
	// Avoiding addition of empty vector to m_removedItems
	if (m_items.find(msg_id) != m_items.end() && !m_items[msg_id].emptyRemoved())
		return m_items[msg_id].m_removedItems;
	else
		return std::vector<IddiffMessage *>();
}

std::vector<IddiffMessage *> Iddiffer::findAdded(int msg_id)
{
	// Avoiding addition of empty vector to m_addedItems
	if (m_items.find(msg_id) != m_items.end() && !m_items[msg_id].emptyAdded())
		return m_items[msg_id].m_addedItems;
	else
		return std::vector<IddiffMessage *>();
}

IddiffMessage *Iddiffer::findAddedSingle(int msg_id)
{
	std::vector<IddiffMessage *> arr = findAdded(msg_id);

	assert(arr.size() <= 1);
	return arr.size() == 1 ? arr[0] : NULL;
}

IddiffMessage *Iddiffer::findRemovedSingle(int msg_id)
{
	std::vector<IddiffMessage *> arr = findRemoved(msg_id);

	assert(arr.size() <= 1);
	return arr.size() == 1 ? arr[0] : NULL;
}

/**
 * \static
 */
IddiffMessage *Iddiffer::findIddiffMessageList(std::vector<IddiffMessage *> list, const IddiffMessage *item)
{
	for (size_t i = 0; i < list.size(); i ++)
		if (list[i]->equalMsgstr(item))
			return list[i];
	// There can't be duplicate items, so the first found item is the only matching one

	return NULL;
}

IddiffMessage *Iddiffer::findRemoved(int msg_id, const IddiffMessage *item)
{
	return findIddiffMessageList(findRemoved(msg_id), item);
}

IddiffMessage *Iddiffer::findRemoved(std::pair<int, IddiffMessage *> item)
{
	return findRemoved(item.first, item.second);
}

IddiffMessage *Iddiffer::findAdded(int msg_id, const IddiffMessage *item)
{
	return findIddiffMessageList(findAdded(msg_id), item);
}

IddiffMessage *Iddiffer::findAdded(std::pair<int, IddiffMessage *> item)
{
	return findAdded(item.first, item.second);
}

void Iddiffer::eraseRemoved(int msg_id, const IddiffMessage *item)
{
	assert(m_items.find(msg_id) != m_items.end());

	m_items[msg_id].eraseRemoved(item);
	cleanupMsgIdData(msg_id);
}

void Iddiffer::eraseAdded(int msg_id, const IddiffMessage *item)
{
	assert(m_items.find(msg_id) != m_items.end());

	m_items[msg_id].eraseAdded(item);
	cleanupMsgIdData(msg_id);
}

const char *Iddiffer::reviewCommentText(int msg_id)
{
	IddiffMessage *comment = m_items[msg_id].m_reviewComment;
	return comment ? comment->msgstr(0) : NULL;
}

/**
 * Does not take ownership of "item".
 */
// 1. remove matching from m_removedItems (if any)
// 2. move all _other_ translations for this "msg_id" from m_addedItems to m_removedItems (there can be only one translation for this "msg_id" in m_addedItems)
// 3. add to m_addedItems
void Iddiffer::acceptTranslation(int msg_id, const IddiffMessage *item)
{
	assert(msg_id);
	assert(item);

	IddiffMessage *removed = findRemoved(msg_id, item);
	if (removed)
		eraseRemoved(msg_id, removed);


	IddiffMessage *added = findAddedSingle(msg_id);
	if (!added)
	{
		// create new item
		insertAdded(msg_id, new IddiffMessage(*item));
	}
	else if (!added->equalMsgstr(item))
	{
		// move old item to "REMOVED", create new item
		insertAdded(msg_id, new IddiffMessage(*item));
		rejectTranslation(msg_id, added);
	}
}

/**
 * Does not take ownership of "item".
 */
// 1. remove matching from m_addedItems (if any)
// 2. add to m_removedItems
void Iddiffer::rejectTranslation(int msg_id, const IddiffMessage *item)
{
	assert(msg_id);
	assert(item);

	IddiffMessage *added = findAddedSingle(msg_id);
	if (added && added->equalMsgstr(item))
		eraseAdded(msg_id, added);


	IddiffMessage *removed = findRemoved(msg_id, item);
	if (!removed)
		insertRemoved(msg_id, new IddiffMessage(*item));
}

/**
 * \brief Returns true if the "accept" action has already been reviewed.
 */
bool Iddiffer::isAcceptAlreadyReviewed(int msg_id, IddiffMessage *item)
{
	return findAdded(msg_id, item) != NULL;
}

/**
 * \brief Returns true if the "reject" action has already been reviewed.
 */
bool Iddiffer::isRejectAlreadyReviewed(int msg_id, IddiffMessage *item)
{
	return findRemoved(msg_id, item) != NULL;
}

void Iddiffer::setCurrentDateTime()
{
	m_date.setCurrentDateTime();
}

// Why this is called "trusted" filtering:
//     If a string was in "ADDED" in "stupids-rerere",
//     the filter still accepts items from "input_diff" with
//     the same translation "REMOVED" (and vice versa).
//     I.e. trusted patches can overwrite data in "stupids-rerere".
//
// This is opposite to "3rd-party idpatch filtering".
void Iddiffer::filterTrustedIddiff(Iddiffer *filter, Iddiffer *input_diff)
{
	if (!input_diff->getReviewVector().empty())
	{
		fprintf(stderr, "Warning: TODO: filter review items\n");
		assert(0);
	}

	std::vector<std::pair<int, IddiffMessage *> > other_removed = input_diff->getRemovedVector();
	std::vector<std::pair<int, IddiffMessage *> > other_added = input_diff->getAddedVector();

	for (size_t i = 0; i < other_removed.size(); i ++)
		if (!filter->findRemoved(other_removed[i]))
			insertRemovedClone(other_removed[i]);

	for (size_t i = 0; i < other_added.size(); i ++)
		if (!filter->findAdded(other_added[i]))
			insertAddedClone(other_added[i]);

	mergeHeaders(input_diff);
}

