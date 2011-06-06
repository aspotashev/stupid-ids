
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

// Escape special symbols and put in quotes.
//
// " -> \"
// newline [\n] -> \n
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

bool IddiffMessage::equalTranslations(const Message *message)
{
	if (numPlurals() != message->numPlurals())
		return false;

	for (int i = 0; i < numPlurals(); i ++)
		if (strcmp(msgstr(i), message->msgstr(i)) != 0)
			return false;

	return true;
}

//----------------------------------------------

Iddiffer::Iddiffer():
	m_output(std::ostringstream::out), m_minimizedIds(false)
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
		m_output << list[i].first << " " << list[i].second->formatPoMessage() << "\n";
}

// This function fills m_removedList and m_addedList
void Iddiffer::diffFiles(TranslationContent *content_a, TranslationContent *content_b)
{
	m_removedList.clear();
	m_addedList.clear();

	// .po files should be derived from the same .pot
	const git_oid *tp_hash = content_a->calculateTpHash();
	assert(git_oid_cmp(tp_hash, content_b->calculateTpHash()) == 0);

	// first_id is the same for 2 files
	int first_id = stupidsClient.getFirstId(tp_hash);


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

std::string Iddiffer::generateIddiffText()
{
	m_output.str("");
	if (m_removedList.size() > 0 || m_addedList.size() > 0)
	{
		m_output << "Subject: " << m_subject << "\n";
		m_output << "Author: " << m_author << "\n";
		m_output << "Date: " << "\n\n"; // TODO: m_date
		m_output << "REMOVED\n";
		writeMessageList(m_removedList);
		m_output << "ADDED\n";
		writeMessageList(m_addedList);
	}

	return m_output.str();
}

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
		else
		{
			if (current_section == SECTION_REMOVED)
				loadMessageListEntry(line, m_removedList);
			else if (current_section == SECTION_ADDED)
				loadMessageListEntry(line, m_addedList);
			else
			{
				// Unknown iddiff section
				assert(0);
			}
		}
	}

	// Check that the .iddiff was not useless
	assert(has_removed || has_added);


	fclose(f);
}

void Iddiffer::loadMessageListEntry(const char *line, std::vector<std::pair<int, IddiffMessage *> > &list)
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
	list.push_back(std::make_pair<int, IddiffMessage *>(msg_id, msg));
}

// Returns _sorted_ vector
std::vector<int> Iddiffer::involvedIds()
{
	std::vector<int> res;
	for (size_t i = 0; i < m_removedList.size(); i ++)
		res.push_back(m_removedList[i].first);
	for (size_t i = 0; i < m_addedList.size(); i ++)
		res.push_back(m_addedList[i].first);

	// sort-uniq
	// TODO: create a function for this
	sort(res.begin(), res.end());
	res.resize(unique(res.begin(), res.end()) - res.begin());

	return res;
}

void Iddiffer::minimizeIds()
{
	std::vector<int> msg_ids_arr = involvedIds();

	// Request minimized IDs from server
	std::vector<int> min_ids_arr = stupidsClient.getMinIds(msg_ids_arr);
	assert(msg_ids_arr.size() == min_ids_arr.size());

	std::map<int, int> min_ids;
	for (size_t i = 0; i < msg_ids_arr.size(); i ++)
		min_ids[msg_ids_arr[i]] = min_ids_arr[i];

	// Write minimized IDs back to m_removedList and m_addedList
	for (size_t i = 0; i < m_removedList.size(); i ++)
		m_removedList[i].first = min_ids[m_removedList[i].first];
	for (size_t i = 0; i < m_addedList.size(); i ++)
		m_addedList[i].first = min_ids[m_addedList[i].first];

	m_minimizedIds = true;
}

std::vector<IddiffMessage *> Iddiffer::getIddiffArr(std::vector<std::pair<int, IddiffMessage *> > &section, int msg_id)
{
	std::vector<IddiffMessage *> res;
	for (size_t i = 0; i < section.size(); i ++)
		if (section[i].first == msg_id)
			res.push_back(section[i].second);

	return res;
}

std::vector<IddiffMessage *> Iddiffer::getRemovedArr(int msg_id)
{
	return getIddiffArr(m_removedList, msg_id);
}

std::vector<IddiffMessage *> Iddiffer::getAddedArr(int msg_id)
{
	return getIddiffArr(m_addedList, msg_id);
}

IddiffMessage *Iddiffer::getAdded(int msg_id)
{
	std::vector<IddiffMessage *> arr = getAddedArr(msg_id);

	assert(arr.size() <= 1);
	return arr.size() == 1 ? arr[0] : NULL;
}

void Iddiffer::applyToMessage(MessageGroup *messageGroup, int min_id)
{
	assert(messageGroup->size() == 1);
	Message *message = messageGroup->message(0);

	std::vector<IddiffMessage *> removed = getRemovedArr(min_id);
	IddiffMessage *added = getAdded(min_id);

	// If "ignoreOldTranslation" is true, we can put the new translation
	bool ignoreOldTranslation = message->isUntranslated();
	for (size_t i = 0; !ignoreOldTranslation && i < removed.size(); i ++)
		if (removed[i]->equalTranslations(message))
			ignoreOldTranslation = true;

	if (ignoreOldTranslation)
	{
		if (added) // change translation
		{
			assert(0);
		}
		else // fuzzy old translation
		{
			assert(0);
		}
	}
	else // !ignoreOldTranslation
	{
		// We cannot just add the new translation without
		// "blacklisting" the existing one.
		assert(!added || added->equalTranslations(message));
	}
}

// Applies the iddiff to the given TranslationContent and writes changes to file
void Iddiffer::applyToContent(TranslationContent *content)
{
	std::vector<MessageGroup *> messages = content->readMessages(false);
	std::vector<int> min_ids = content->getMinIds();
	std::vector<int> involved_ids = involvedIds();

	for (size_t i = 0; i < messages.size(); i ++)
		if (binary_search(involved_ids.begin(), involved_ids.end(), min_ids[i]))
			applyToMessage(messages[i], min_ids[i]);

	// TODO: write content back to file
	assert(0);
}

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

