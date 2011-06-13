
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <algorithm>

#include <git2.h>
#include <gettextpo-helper/gettextpo-helper.h>
#include <gettextpo-helper/translationcontent.h>
#include <gettextpo-helper/stupids-client.h>
#include <gettextpo-helper/translation-collector.h>

#include <gettextpo-helper/iddiff.h>


IddiffMessage::IddiffMessage()
{
	m_numPlurals = 0;
	m_fuzzy = false;
}

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

void IddiffMessage::addMsgstr(const char *str)
{
	assert(m_numPlurals < MAX_PLURAL_FORMS);
	m_numPlurals ++;

	m_msgstr[m_numPlurals - 1] = xstrdup(str);
}

bool IddiffMessage::isFuzzy() const
{
	return m_fuzzy;
}

void IddiffMessage::setFuzzy(bool fuzzy)
{
	m_fuzzy = fuzzy;
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

/**
 * \brief Escape special symbols and put in quotes.
 *
 * Escape the following characters: double quote ("), newline, tab, backslash.
 *
 * \static
 */
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
		else if (str[i] == '\\')
			res += "\\\\";
		else if (str[i] == '\n')
			res += "\\n";
		else if (str[i] == '\t')
			res += "\\t";
		else if ((unsigned char)str[i] < ' ')
		{
			printf("Unescaped special symbol: code = %d\n", (int)str[i]);
			assert(0);
		}
		else
			res += str[i];
	}

	res += "\"";
	return res;
}

bool IddiffMessage::equalTranslations(const IddiffMessage *message) const
{
	if (numPlurals() != message->numPlurals())
		return false;

	for (int i = 0; i < numPlurals(); i ++)
		if (strcmp(msgstr(i), message->msgstr(i)) != 0)
			return false;

	return true;
}

bool IddiffMessage::equalTranslations(const Message *message) const
{
	if (numPlurals() != message->numPlurals())
		return false;

	for (int i = 0; i < numPlurals(); i ++)
		if (strcmp(msgstr(i), message->msgstr(i)) != 0)
			return false;

	return true;
}

/**
 * Fuzzy flag state will not be copied.
 */
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

Iddiffer::Iddiffer():
	m_output(std::ostringstream::out), m_minimizedIds(false)
{
}

Iddiffer::~Iddiffer()
{
}

/**
 * \static
 */
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
		m_output << list[i].first << " " << list[i].second->formatPoMessage() << "\n";
}

void Iddiffer::clearIddiff()
{
	m_removedItems.clear();
	m_addedItems.clear();
}

// This function fills m_removedItems and m_addedItems
void Iddiffer::diffAgainstEmpty(TranslationContent *content_b)
{
	clearIddiff();

	const git_oid *tp_hash = content_b->calculateTpHash();
	int first_id = stupidsClient.getFirstId(tp_hash);
	assert(first_id > 0);


	// compare pairs of messages in 2 .po files
	po_file_t file_b = content_b->poFileRead();
	po_message_iterator_t iterator_b = po_message_iterator(file_b, "messages");
	// skipping headers
	po_message_t message_b = po_next_message(iterator_b);

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
void Iddiffer::diffFiles(TranslationContent *content_a, TranslationContent *content_b)
{
	clearIddiff();

	// .po files should be derived from the same .pot
	const git_oid *tp_hash = content_a->calculateTpHash();
	assert(git_oid_cmp(tp_hash, content_b->calculateTpHash()) == 0);

	// first_id is the same for 2 files
	int first_id = stupidsClient.getFirstId(tp_hash);
	assert(first_id > 0);


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
			fprintf(stderr, "Changes in comments will be ignored!\n");
			fprintf(stderr, "<<<<<\n");
			fprintf(stderr, "%s", po_message_comments(message_a)); // "\n" should be included in comments
			fprintf(stderr, "=====\n");
			fprintf(stderr, "%s", po_message_comments(message_b)); // "\n" should be included in comments
			fprintf(stderr, ">>>>>\n");
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

std::string Iddiffer::generateIddiffText()
{
	m_output.str("");
	std::vector<std::pair<int, IddiffMessage *> > removed_list = getRemovedVector();
	std::vector<std::pair<int, IddiffMessage *> > added_list = getAddedVector();
	if (removed_list.size() > 0 || added_list.size() > 0)
	{
		m_output << "Subject: " << m_subject << "\n";
		m_output << "Author: " << m_author << "\n";
		m_output << "Date: " << "\n\n"; // TODO: m_date
		m_output << "REMOVED\n";
		writeMessageList(removed_list);
		m_output << "ADDED\n";
		writeMessageList(added_list);
	}

	return m_output.str();
}

/**
 * \static
 */
std::string Iddiffer::generateIddiffText(TranslationContent *content_a, TranslationContent *content_b)
{
	Iddiffer *diff = new Iddiffer();
	diff->diffFiles(content_a, content_b);

	std::string res = diff->generateIddiffText();
	delete diff;

	return res;
}

void Iddiffer::loadIddiff(const char *filename)
{
	FILE *f = fopen(filename, "r");
	fseek(f, 0, SEEK_END);
	int file_size = (int)ftell(f);
	rewind(f);

	char *buffer = new char[file_size + 1];
	assert(fread(buffer, 1, file_size, f) == file_size);
	buffer[file_size] = '\0';

	std::vector<char *> lines;

	char *cur_line = buffer;
	char *line_break = buffer; // something that is not NULL

	bool has_subject = false;
	bool has_author = false;
	bool has_date = false;
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
			assert(!has_date); // catch duplicate "Date:" fields
			has_date = true;

			// TODO: m_date
		}
		else
		{
			// Unknown header field
			assert(0);
		}
	}
	// Check that all fields are present (is it really necessary?)
	assert(has_subject);
	assert(has_author);
	assert(has_date);


	enum
	{
		SECTION_UNKNOWN,
		SECTION_REMOVED,
		SECTION_ADDED
	} current_section = SECTION_UNKNOWN;

	bool has_removed = false;
	bool has_added = false;
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
		// Now we are processing a line inside a section
		else if (current_section == SECTION_REMOVED)
			insertRemoved(loadMessageListEntry(line));
		else if (current_section == SECTION_ADDED)
			insertAdded(loadMessageListEntry(line));
		else // invalid value of "current_section"
		{
			// Unknown iddiff section
			assert(0);
		}
	}

	// Check that the .iddiff was not useless
	assert(has_removed || has_added);


	fclose(f);
}

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

// Returns _sorted_ vector
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

	// sort-uniq
	// TODO: create a function for this
	sort(res.begin(), res.end());
	res.resize(unique(res.begin(), res.end()) - res.begin());

	return res;
}

void Iddiffer::substituteMsgId(std::map<int, std::vector<IddiffMessage *> > &items, int old_id, int new_id)
{
	// Check that IDs won't collide
	assert(items.find(new_id) == items.end());

	if (items.find(old_id) != items.end())
	{
		items[new_id] = items[old_id]; // copying vector
		items.erase(old_id);
	}
}

void Iddiffer::substituteMsgId(int old_id, int new_id)
{
	substituteMsgId(m_removedItems, old_id, new_id);
	substituteMsgId(m_addedItems, old_id, new_id);
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

void Iddiffer::applyToMessage(MessageGroup *messageGroup, int min_id)
{
	assert(messageGroup->size() == 1);
	Message *message = messageGroup->message(0);

	std::vector<IddiffMessage *> removed = findRemoved(min_id);
	IddiffMessage *added = findAddedSingle(min_id);

	// If "ignoreOldTranslation" is true, we can put the new translation
	bool ignoreOldTranslation = message->isUntranslated();
	for (size_t i = 0; !ignoreOldTranslation && i < removed.size(); i ++)
		if (removed[i]->equalTranslations(message))
			ignoreOldTranslation = true;

	if (ignoreOldTranslation)
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
	else // !ignoreOldTranslation
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
	std::vector<MessageGroup *> messages = content->readMessages(false);
	std::vector<int> min_ids = content->getMinIds();
	std::vector<int> involved_ids = involvedIds();

	for (size_t i = 0; i < messages.size(); i ++)
		if (binary_search(involved_ids.begin(), involved_ids.end(), min_ids[i]))
			applyToMessage(messages[i], min_ids[i]);

	// Write updated content back to file
	content->writeToFile(); // TODO: StupIdTranslationCollector::writeChanges() for writing all changes to .po files
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
		applyToContent(contents[i]); // writes changes to file
	}
}

/**
 * \brief Add translation version to the "REMOVED" section ensuring that it does not conflict with existing Iddiff entries.
 */
void Iddiffer::insertRemoved(int msg_id, const IddiffMessage *item)
{
	assert(findRemoved(msg_id, item) == NULL); // duplicate in "REMOVED"
	assert(findAdded(msg_id, item) == NULL); // conflict: trying to "REMOVE" a translation already existing in "ADDED"

	m_removedItems[msg_id].push_back(new IddiffMessage(*item));
}

/**
 * \brief Add translation version to the "ADDED" section ensuring that it does not conflict with existing Iddiff entries.
 */
void Iddiffer::insertAdded(int msg_id, const IddiffMessage *item)
{
	assert(findRemoved(msg_id, item) == NULL); // conflict: trying to "REMOVE" a translation already existing in "ADDED"

	std::vector<IddiffMessage *> added_this_id = findAdded(msg_id);
	for (size_t i = 0; i < added_this_id.size(); i ++)
	{
		if (added_this_id[i]->equalTranslations(item))
			assert(0); // duplicate in "ADDED"
		else
			assert(0); // conflict: two different translations in "ADDED"
	}

	m_addedItems[msg_id].push_back(new IddiffMessage(*item));
}

void Iddiffer::insertRemoved(std::pair<int, IddiffMessage *> item)
{
	insertRemoved(item.first, item.second);
}

void Iddiffer::insertAdded(std::pair<int, IddiffMessage *> item)
{
	insertAdded(item.first, item.second);
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

void Iddiffer::merge(Iddiffer *diff)
{
	std::vector<std::pair<int, IddiffMessage *> > other_removed = diff->getRemovedVector();
	std::vector<std::pair<int, IddiffMessage *> > other_added = diff->getAddedVector();

	for (size_t i = 0; i < other_removed.size(); i ++)
		insertRemoved(other_removed[i].first, other_removed[i].second);
	for (size_t i = 0; i < other_added.size(); i ++)
		insertAdded(other_added[i].first, other_added[i].second);
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
		if (list[i]->equalTranslations(item))
			return list[i];
	// There can't be duplicate items, so the first found item is the only matching one

	return NULL;
}

IddiffMessage *Iddiffer::findRemoved(int msg_id, const IddiffMessage *item)
{
	return findIddiffMessageList(findRemoved(msg_id), item);
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

// 1. remove matching from m_removedItems (if any)
// 2. move all _other_ translations for this "msg_id" from m_addedItems to m_removedItems (there can be only one translation for this "msg_id" in m_addedItems)
// 3. add to m_addedItems
void Iddiffer::acceptTranslation(int msg_id, IddiffMessage *item)
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
		insertAdded(msg_id, item);
	}
	else if (!added->equalTranslations(item))
	{
		// move old item to "REMOVED", create new item
		insertAdded(msg_id, item);
		rejectTranslation(msg_id, added);
	}
}

// 1. remove matching from m_addedItems (if any)
// 2. add to m_removedItems
void Iddiffer::rejectTranslation(int msg_id, IddiffMessage *item)
{
	assert(msg_id);
	assert(item);

	IddiffMessage *added = findAddedSingle(msg_id);
	if (added && added->equalTranslations(item))
		eraseAdded(msg_id, added);


	IddiffMessage *removed = findRemoved(msg_id, item);
	if (!removed)
		insertRemoved(msg_id, item);
}

