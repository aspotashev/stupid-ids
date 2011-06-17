
#include <gettextpo-helper/message.h>
#include <gettextpo-helper/iddiff.h>

//------ C++ wrapper library for 'libgettextpo' with some extra features -------

// "packed" means that 'n_plurals' contains more information than m_numPlurals:
// 	1. whether the message uses plural forms (n_plurals=0 means that message does not use plural forms)
// 	2. the number of plural forms
void Message::setNPluralsPacked(int n_plurals)
{
	m_numPlurals = n_plurals;
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
}

Message::Message(po_message_t message, int index, const char *filename):
	m_index(index),
	m_filename(xstrdup(filename))
{
	clear();
	setNPluralsPacked(po_message_n_plurals(message));

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

void Message::setMsgstr(int index, const char *str)
{
	assert(m_msgstr[index] == 0);

	m_msgstr[index] = xstrdup(str);
}

// TODO: make sure that all instance (m_*) variables are initialized
Message::Message(bool fuzzy, const char *msgcomment, const char *msgstr0, int n_plurals):
	m_index(-1), m_filename(NULL)
{
	m_obsolete = false;

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
	m_msgcomments = 0;
	for (int i = 0; i < MAX_PLURAL_FORMS; i ++)
		m_msgstr[i] = 0;

	m_edited = false;
}

// TODO: make sure that all instance (m_*) variables are initialized
Message::Message(bool fuzzy, int n_plurals, const char *msgcomment):
	m_index(-1), m_filename(NULL)
{
	clear();

	m_fuzzy = fuzzy;
	setNPluralsPacked(n_plurals);
	setMsgcomments(msgcomment);
}

Message::~Message()
{
	if (m_filename)
		delete [] m_filename;

	// TODO: free memory
}

int Message::index() const
{
	return m_index;
}

const char *Message::filename() const
{
	return m_filename;
}

bool Message::isFuzzy() const
{
	return m_fuzzy;
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

int Message::numPlurals() const
{
	return m_numPlurals;
}

const char *Message::msgstr(int plural_form) const
{
	assert(plural_form >= 0 && plural_form < m_numPlurals);

	return m_msgstr[plural_form];
}

const char *Message::msgcomments() const
{
	return m_msgcomments;
}

// Returns whether msgstr[*] and translator's comments are equal in two messages.
// States of 'fuzzy' flag should also be the same.
bool Message::equalTranslations(const Message *o) const
{
	assert(m_plural == o->isPlural());

	// This happens, see messages/kdebase/plasma_applet_trash.po
	// from http://websvn.kde.org/?view=revision&revision=825044
	if (m_numPlurals != o->numPlurals())
		return false;

	for (int i = 0; i < m_numPlurals; i ++)
		if (strcmp(m_msgstr[i], o->msgstr(i)))
			return false;

	return m_fuzzy == o->isFuzzy() && !strcmp(m_msgcomments, o->msgcomments());
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

std::string Message::formatPoMessage() const
{
	std::string res;

	if (isFuzzy())
		res += "f";

	res += IddiffMessage::formatString(msgstr(0));
	for (int i = 1; i < numPlurals(); i ++)
	{
		res += " "; // separator
		res += IddiffMessage::formatString(msgstr(i));
	}

	return res;
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
	// TODO: free memory. Who takes care of "Message" objects?
}

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

void MessageGroup::mergeMessageGroup(MessageGroup *other)
{
	// Only message groups with the same
	// {msgid, msgid_plural, msgctxt} can be merged.
//	TODO:
//	assert(!strcmp(msgid(), other->msgid()));
//	...

	for (int i = 0; i < other->size(); i ++)
		addMessage(other->message(i));
}

void MessageGroup::clear()
{
	m_msgid = NULL;
	m_msgidPlural = NULL;
	m_msgctxt = NULL;
}

void MessageGroup::setMsgid(const char *str)
{
	assert(m_msgid == NULL);

	m_msgid = xstrdup(str);
}

void MessageGroup::setMsgidPlural(const char *str)
{
	assert(m_msgidPlural == NULL);

	m_msgidPlural = xstrdup(str);
}

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

