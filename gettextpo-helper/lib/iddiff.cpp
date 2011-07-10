
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

IddiffMessage::IddiffMessage (const IddiffMessage &msg)
{
	m_fuzzy = msg.m_fuzzy;

	m_numPlurals = msg.m_numPlurals;
	for (int i = 0; i < m_numPlurals; i ++)
		m_msgstr[i] = xstrdup(msg.m_msgstr[i]);
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
	m_removedItems.clear();
	m_addedItems.clear();
	m_reviewComments.clear();
}

void Iddiffer::clearReviewComments()
{
	m_reviewComments.clear();
}

void Iddiffer::clearReviewComment(int msg_id)
{
	m_reviewComments.erase(msg_id);
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
			insertRemoved(first_id + index, new IddiffMessage(message_a));

		// Adding to "ADDED" if:
		//    "B" is translated & there were changes (i.e. message_a != message_b)
		if (!po_message_is_untranslated(message_b) && !po_message_is_fuzzy(message_b))
			insertAdded(first_id + index, new IddiffMessage(message_b));
	}

	// free memory
	po_message_iterator_free(iterator_a);
	po_message_iterator_free(iterator_b);
	po_file_free(file_a);
	po_file_free(file_b);
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
	for (std::map<int, std::vector<IddiffMessage *> >::iterator iter = m_removedItems.begin();
		iter != m_removedItems.end(); iter ++)
		res.push_back(iter->first);
	for (std::map<int, std::vector<IddiffMessage *> >::iterator iter = m_addedItems.begin();
		iter != m_addedItems.end(); iter ++)
		res.push_back(iter->first);
	for (std::map<int, IddiffMessage *>::iterator iter = m_reviewComments.begin();
		iter != m_reviewComments.end(); iter ++)
		res.push_back(iter->first);

	sort_uniq(res);
	return res;
}

/**
 * \static
 */
template <typename T>
void Iddiffer::substituteMsgId(std::map<int, T> &items, int old_id, int new_id)
{
	// Check that IDs won't collide
	if (items.find(new_id) != items.end())
	{
		printf("new_id = %d, old_id = %d\n", new_id, old_id);
		assert(0);
	}

	if (items.find(old_id) != items.end())
	{
		items[new_id] = items[old_id]; // copying object of class T
		items.erase(old_id);
	}
}

void Iddiffer::substituteMsgId(int old_id, int new_id)
{
	if (old_id == new_id)
		return;

	substituteMsgId(m_removedItems, old_id, new_id);
	substituteMsgId(m_addedItems, old_id, new_id);
	substituteMsgId(m_reviewComments, old_id, new_id);
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

	if (message->isUntranslated())
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
		}
		else // fuzzy old translation
		{
			message->editFuzzy(true);
		}
	}
	else // !canDropMessage
	{
		// We cannot just add the new translation without
		// "blacklisting" the existing one.
		assert(!added || added->equalTranslations(message));
	}
}

// TODO: rewrite using StupIdTranslationCollector::getMessagesByIds(std::vector<MessageGroup *> &messages, std::vector<TranslationContent *> &contents)
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

// TODO: rewrite using StupIdTranslationCollector::getMessagesByIds(std::vector<MessageGroup *> &messages, std::vector<TranslationContent *> &contents)
void Iddiffer::applyIddiff(StupIdTranslationCollector *collector)
{
	// Check that involvedIds() will return minimized IDs
	assert(m_minimizedIds);

	std::vector<TranslationContent *> contents = collector->involvedByMinIds(involvedIds());
	printf("involved contents: %d\n", (int)contents.size());
	for (size_t i = 0; i < contents.size(); i ++)
	{
		printf("Patching %s\n", contents[i]->displayFilename());
		applyToContent(contents[i]);
		contents[i]->writeToFile(); // TODO: StupIdTranslationCollector::writeChanges() for writing all changes to .po files
	}
}

/**
 * \brief Add translation version to the "REMOVED" section ensuring that it does not conflict with existing Iddiff entries.
 *
 * Takes ownership of "item".
 */
void Iddiffer::insertRemoved(int msg_id, IddiffMessage *item)
{
	assert(findRemoved(msg_id, item) == NULL); // duplicate in "REMOVED"
	assert(findAdded(msg_id, item) == NULL); // conflict: trying to "REMOVE" a translation already existing in "ADDED"

	m_removedItems[msg_id].push_back(item);
}

/**
 * \brief Add translation version to the "ADDED" section ensuring that it does not conflict with existing Iddiff entries.
 *
 * Takes ownership of "item".
 */
void Iddiffer::insertAdded(int msg_id, IddiffMessage *item)
{
	assert(findRemoved(msg_id, item) == NULL); // conflict: trying to "REMOVE" a translation already existing in "ADDED"

	std::vector<IddiffMessage *> added_this_id = findAdded(msg_id);
	for (size_t i = 0; i < added_this_id.size(); i ++)
	{
		if (added_this_id[i]->equalMsgstr(item))
			assert(0); // duplicate in "ADDED"
		else
			assert(0); // conflict: two different translations in "ADDED"
	}

	m_addedItems[msg_id].push_back(item);
}

/**
 * \brief Add message to the "REVIEW" section ensuring that there was no "REVIEW" entry for the given msg_id.
 *
 * Takes ownership of "item".
 */
void Iddiffer::insertReview(int msg_id, IddiffMessage *item)
{
	assert(m_reviewComments[msg_id] == NULL);
	assert(item->isTranslated()); // text should not be empty

	m_reviewComments[msg_id] = item;
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

/**
 * \static
 */
std::vector<std::pair<int, IddiffMessage *> > Iddiffer::getItemsVector(std::map<int, std::vector<IddiffMessage *> > &items)
{
	std::vector<std::pair<int, IddiffMessage *> > res;
	for (std::map<int, std::vector<IddiffMessage *> >::iterator iter = items.begin();
		iter != items.end(); iter ++)
	{
		std::vector<IddiffMessage *> id_items = iter->second;
		for (size_t i = 0; i < id_items.size(); i ++)
			res.push_back(std::make_pair<int, IddiffMessage *>(iter->first, id_items[i]));
	}

	return res;
}

std::vector<std::pair<int, IddiffMessage *> > Iddiffer::getRemovedVector()
{
	return getItemsVector(m_removedItems);
}

std::vector<std::pair<int, IddiffMessage *> > Iddiffer::getAddedVector()
{
	return getItemsVector(m_addedItems);
}

std::vector<std::pair<int, IddiffMessage *> > Iddiffer::getReviewVector()
{
	std::vector<std::pair<int, IddiffMessage *> > res;
	for (std::map<int, IddiffMessage *>::iterator iter = m_reviewComments.begin();
		iter != m_reviewComments.end(); iter ++)
	{
		if (iter->second)
			res.push_back(std::make_pair<int, IddiffMessage *>(iter->first, iter->second));
	}

	return res;
}

void Iddiffer::mergeHeaders(Iddiffer *diff)
{
	// Keep the most recent date/time
	if (m_date.isNull() || diff->date() > m_date)
		m_date = diff->date();

	if (m_author.empty())
		m_author = diff->m_author;
	else if (!diff->m_author.empty())
		m_author += ", " + diff->m_author;
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
	if (m_removedItems.find(msg_id) != m_removedItems.end())
		return m_removedItems[msg_id];
	else
		return std::vector<IddiffMessage *>();
}

std::vector<IddiffMessage *> Iddiffer::findAdded(int msg_id)
{
	// Avoiding addition of empty vector to m_addedItems
	if (m_addedItems.find(msg_id) != m_addedItems.end())
		return m_addedItems[msg_id];
	else
		return std::vector<IddiffMessage *>();
}

IddiffMessage *Iddiffer::findAddedSingle(int msg_id)
{
	std::vector<IddiffMessage *> arr = findAdded(msg_id);

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

/**
 * \static
 */
void Iddiffer::eraseItem(std::map<int, std::vector<IddiffMessage *> > &items, int msg_id, const IddiffMessage *item)
{
	assert(items.find(msg_id) != items.end());

	std::vector<IddiffMessage *> &list = items[msg_id];
	for (std::vector<IddiffMessage *>::iterator iter = list.begin(); iter != list.end(); iter ++)
		if (*iter == item) // comparison by pointer, not by translations!
		{
			list.erase(iter);
			break;
		}

	// Do not keep empty vectors
	if (list.size() == 0)
		items.erase(msg_id);
}

void Iddiffer::eraseRemoved(int msg_id, const IddiffMessage *item)
{
	eraseItem(m_removedItems, msg_id, item);
}

void Iddiffer::eraseAdded(int msg_id, const IddiffMessage *item)
{
	eraseItem(m_addedItems, msg_id, item);
}

const char *Iddiffer::reviewCommentText(int msg_id)
{
	IddiffMessage *comment = m_reviewComments[msg_id];
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

