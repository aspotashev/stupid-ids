
#include <stdio.h>

#include <gettextpo-helper/message.h>
#include <gettextpo-helper/iddiff.h>


MessageTranslationBase::MessageTranslationBase()
{
	MessageTranslationBase::clear();
}

MessageTranslationBase::MessageTranslationBase(po_message_t message)
{
	for (int i = 0; i < MAX_PLURAL_FORMS; i ++)
		m_msgstr[i] = 0;

	// set m_numPlurals
	bool m_plural = setNPluralsPacked(po_message_n_plurals(message));

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

MessageTranslationBase::~MessageTranslationBase()
{
	for (int i = 0; i < m_numPlurals; i ++)
		delete [] m_msgstr[i];
}

void MessageTranslationBase::clear()
{
	m_numPlurals = 0;
	for (int i = 0; i < MAX_PLURAL_FORMS; i ++)
		m_msgstr[i] = NULL;

	m_fuzzy = false;
}

// "packed" means that 'n_plurals' contains more information than m_numPlurals:
// 	1. whether the message uses plural forms (n_plurals=0 means that message does not use plural forms)
// 	2. the number of plural forms
//
// Returns whether the message uses plural forms (true or false).
bool MessageTranslationBase::setNPluralsPacked(int n_plurals)
{
	assert(n_plurals >= 0);

	// n_plurals=0 means that there is only one form "msgstr"
	m_numPlurals = std::max(n_plurals, 1); 
	assert(m_numPlurals <= MAX_PLURAL_FORMS); // limited by the size of m_msgstr

	// Return whether the message uses plural forms
	return n_plurals > 0;
}

/**
 * \static
 */
std::string MessageTranslationBase::formatPoMessage(po_message_t message)
{
	MessageTranslationBase *msg = new MessageTranslationBase(message);
	std::string res = msg->formatPoMessage();
	delete msg;

	return res;
}

std::string MessageTranslationBase::formatPoMessage() const
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
std::string MessageTranslationBase::formatString(const char *str)
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

bool MessageTranslationBase::isFuzzy() const
{
	return m_fuzzy;
}

const char *MessageTranslationBase::msgstr(int plural_form) const
{
	assert(plural_form >= 0 && plural_form < m_numPlurals);

	return m_msgstr[plural_form];
}

void MessageTranslationBase::setMsgstr(int index, const char *str)
{
	assert(m_msgstr[index] == 0);

	m_msgstr[index] = xstrdup(str);
}

int MessageTranslationBase::numPlurals() const
{
	return m_numPlurals;
}

// Returns whether msgstr[*] are equal in two messages.
bool MessageTranslationBase::equalMsgstr(const MessageTranslationBase *o) const
{
	// This happens, see messages/kdebase/plasma_applet_trash.po
	// from http://websvn.kde.org/?view=revision&revision=825044
	if (m_numPlurals != o->numPlurals())
		return false;

	for (int i = 0; i < m_numPlurals; i ++)
		if (strcmp(m_msgstr[i], o->msgstr(i)))
			return false;

	return true;
}

// Returns whether msgstr[*] are equal in two messages.
// States of 'fuzzy' flag should also be the same.
bool MessageTranslationBase::equalTranslations(const MessageTranslationBase *o) const
{
	return equalMsgstr(o) && m_fuzzy == o->isFuzzy();
}

//-------------------------------------------------------

Message::Message(const Message &message)
{
	clear();

	assert(message.m_numPlurals <= MAX_PLURAL_FORMS);

	// Fields inherited from class MessageTranslationBase
	m_numPlurals = message.m_numPlurals;
	for (int i = 0; i < m_numPlurals; i ++)
	{
		assert(message.m_msgstr[i]);
		m_msgstr[i] = xstrdup(message.m_msgstr[i]);
	}
	m_fuzzy = message.m_fuzzy;

	// Fields defined in this class
	m_plural = message.m_plural;
	m_obsolete = message.m_obsolete;
	m_untranslated = message.m_untranslated;
	if (message.m_msgcomments)
		m_msgcomments = xstrdup(message.m_msgcomments);
	m_index = message.m_index;
	if (message.m_filename)
		m_filename = xstrdup(message.m_filename);
	m_edited = message.m_edited;
}

// Cannot use simple MessageTranslationBase(po_message_t),
// because it does not set m_plural.
Message::Message(po_message_t message, int index, const char *filename)
{
	clear();

	m_index = index;
	m_filename = xstrdup(filename);
	m_plural = setNPluralsPacked(po_message_n_plurals(message));

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
	m_obsolete = po_message_is_obsolete(message) != 0;

	// TODO: write and use another Message:: function for this (should not use 'po_message_t message')
	m_untranslated = po_message_is_untranslated(message);

	setMsgcomments(po_message_comments(message));
}

// translators' comments
void Message::setMsgcomments(const char *str)
{
	assert(m_msgcomments == 0);

	m_msgcomments = xstrdup(str);
}

Message::Message(bool fuzzy, const char *msgcomment, const char *msgstr0, int n_plurals)
{
	clear();

	m_fuzzy = fuzzy;
	setMsgcomments(msgcomment);

	// n_plurals=1 means that there is only msgstr[0]
	// n_plurals=0 means that there is only msgstr (and the message is not pluralized)
	assert(n_plurals == 0 || n_plurals == 1);

	m_numPlurals = 1;
	m_plural = (n_plurals == 1);
	setMsgstr(0, msgstr0);
}

void Message::clear()
{
	MessageTranslationBase::clear();

	m_plural = false;
	m_obsolete = false;
	m_untranslated = false;
	m_msgcomments = NULL;

	m_index = -1;
	m_filename = NULL;

	m_edited = false;
}

Message::Message(bool fuzzy, int n_plurals, const char *msgcomment)
{
	clear();

	m_fuzzy = fuzzy;
	m_plural = setNPluralsPacked(n_plurals);
	setMsgcomments(msgcomment);
}

Message::~Message()
{
	if (m_filename)
		delete [] m_filename;
	if (m_msgcomments)
		delete [] m_msgcomments;
}

int Message::index() const
{
	return m_index;
}

const char *Message::filename() const
{
	return m_filename;
}

bool Message::isPlural() const
{
	return m_plural;
}

bool Message::isUntranslated() const
{
	return m_untranslated;
}

/**
 * \brief Not untranslated and not fuzzy.
 */
bool Message::isTranslated() const
{
	return !isFuzzy() && !isUntranslated();
}

const char *Message::msgcomments() const
{
	return m_msgcomments;
}

void Message::editFuzzy(bool fuzzy)
{
	if (fuzzy == m_fuzzy)
		return;

	m_fuzzy = fuzzy;
	m_edited = true;
}

void Message::editMsgstr(int index, const char *str)
{
	assert(index >= 0 && index < numPlurals());
	assert(m_msgstr[index]);

	if (strcmp(m_msgstr[index], str) == 0)
		return;

	delete [] m_msgstr[index];
	m_msgstr[index] = xstrdup(str);
	m_edited = true;
}

bool Message::isEdited() const
{
	return m_edited;
}

// Returns whether msgstr[*] and translator's comments are equal in two messages.
// States of 'fuzzy' flag should also be the same.
bool Message::equalTranslationsComments(const Message *o) const
{
	assert(m_plural == o->isPlural());

	return MessageTranslationBase::equalTranslations(o) && !strcmp(m_msgcomments, o->msgcomments());
}

//-------------------------------------------------------

MessageGroup::MessageGroup()
{
	clear();
}

MessageGroup::MessageGroup(po_message_t message, int index, const char *filename)
{
	clear();
	setMsgid(po_message_msgid(message));
	if (po_message_msgid_plural(message))
		setMsgidPlural(po_message_msgid_plural(message));
	if (po_message_msgctxt(message))
		setMsgctxt(po_message_msgctxt(message));

	addMessage(new Message(message, index, filename));
}

MessageGroup::~MessageGroup()
{
	if (m_msgid)
		delete [] m_msgid;
	if (m_msgidPlural)
		delete [] m_msgidPlural;
	if (m_msgctxt)
		delete [] m_msgctxt;

	for (size_t i = 0; i < m_messages.size(); i ++)
		delete m_messages[i];
}

/**
 * Takes ownership of "message".
 */
void MessageGroup::addMessage(Message *message)
{
	m_messages.push_back(message);
}

int MessageGroup::size() const
{
	return (int)m_messages.size();
}

Message *MessageGroup::message(int index)
{
	assert(index >= 0 && index < size());

	return m_messages[index];
}

const Message *MessageGroup::message(int index) const
{
	assert(index >= 0 && index < size());

	return m_messages[index];
}

/**
 * Does not take ownership of messages from "other" MessageGroup.
 */
// TODO: "mergeMessageGroupNokeep" that removed messages from "other" MessageGroup
void MessageGroup::mergeMessageGroup(const MessageGroup *other)
{
	// Only message groups with the same
	// {msgid, msgid_plural, msgctxt} can be merged.
//	TODO:
//	assert(!strcmp(msgid(), other->msgid()));
//	...

	for (int i = 0; i < other->size(); i ++)
		addMessage(new Message(*other->message(i)));
}

void MessageGroup::clear()
{
	m_msgid = NULL;
	m_msgidPlural = NULL;
	m_msgctxt = NULL;
}

/**
 * Does not take ownership of "str".
 */
void MessageGroup::setMsgid(const char *str)
{
	assert(m_msgid == NULL);

	m_msgid = xstrdup(str);
}

/**
 * Does not take ownership of "str".
 */
void MessageGroup::setMsgidPlural(const char *str)
{
	assert(m_msgidPlural == NULL);

	m_msgidPlural = xstrdup(str);
}

/**
 * Does not take ownership of "str".
 */
void MessageGroup::setMsgctxt(const char *str)
{
	assert(m_msgctxt == NULL);

	m_msgctxt = xstrdup(str);
}

const char *MessageGroup::msgid() const
{
	assert(m_msgid);

	return m_msgid;
}

/**
 * \brief Returns NULL for messages without plural forms.
 */
const char *MessageGroup::msgidPlural() const
{
	// Checking that m_msgid is initialized. This should mean
	// that m_msgidPlural is also set (probably, set to NULL).
	assert(m_msgid);

	return m_msgidPlural;
}

const char *MessageGroup::msgctxt() const
{
	// Checking that m_msgid is initialized. This should mean
	// that m_msgctxt is also set (probably, set to NULL).
	assert(m_msgid);

	return m_msgctxt;
}

