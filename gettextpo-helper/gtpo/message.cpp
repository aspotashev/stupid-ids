#include "message.h"
#include "iddiff.h"

Message::Message(const Message& message)
    : MessageTranslationBase(message) // TBD: make sure this copy ctor works!
    , m_obsolete(false)
    , m_untranslated(false)
    , m_msgcomments(nullptr)
    , m_index(-1)
    , m_filename()
    , m_edited(false)
{
    // Fields defined in this class
    m_obsolete = message.m_obsolete;
    m_untranslated = message.m_untranslated;
    m_msgcomments = message.m_msgcomments;
    m_index = message.m_index;
    m_filename = message.m_filename;
    m_edited = message.m_edited;
}

// Cannot use simple MessageTranslationBase(po_message_t),
// because it does not set m_plural.
Message::Message(po_message_t message, int index, const std::string& filename)
    : MessageTranslationBase(message)
    , m_obsolete(false)
    , m_untranslated(false)
    , m_msgcomments(nullptr)
    , m_index(index)
    , m_filename(filename)
    , m_edited(false)
{
    m_obsolete = po_message_is_obsolete(message) != 0;

    // TODO: write and use another Message:: function for this (should not use 'po_message_t message')
    m_untranslated = po_message_is_untranslated(message);

    setMsgcomments(po_message_comments(message));
}

Message::Message(
    bool fuzzy,
    const std::string& msgcomment,
    const std::string& msgstr0,
    int n_plurals)
    : MessageTranslationBase()
    , m_obsolete(false)
    , m_untranslated(false)
    , m_msgcomments(nullptr)
    , m_index(-1)
    , m_filename()
    , m_edited(false)
{
    m_fuzzy = fuzzy;
    setMsgcomments(msgcomment);

    // n_plurals=1 means that there is only msgstr[0]
    // n_plurals=0 means that there is only msgstr (and the message is not pluralized)
    assert(n_plurals == 0 || n_plurals == 1);

    m_plural = n_plurals > 0;
    m_msgstr.push_back(msgstr0);
}

Message::Message(bool fuzzy, int n_plurals, const std::string& msgcomment)
    : MessageTranslationBase()
    , m_obsolete(false)
    , m_untranslated(false)
    , m_msgcomments(msgcomment)
    , m_index(-1)
    , m_filename()
    , m_edited(false)
{
    m_fuzzy = fuzzy;
    m_plural = n_plurals > 0;
}

Message::~Message()
{
}

// translators' comments
void Message::setMsgcomments(const std::string& str)
{
    assert(m_msgcomments.isNull());

    m_msgcomments = str;
}

// void Message::clear()
// {
//     MessageTranslationBase::clear();
//
//     m_plural = false;
//     m_obsolete = false;
//     m_untranslated = false;
//     m_msgcomments = NULL;
//
//     m_index = -1;
//     m_filename = NULL;
//
//     m_edited = false;
// }

int Message::index() const
{
    return m_index;
}

std::string Message::filename() const
{
    return m_filename;
}

bool Message::isUntranslated() const
{
    return m_untranslated;
}

bool Message::isTranslated() const
{
    return !isFuzzy() && !isUntranslated();
}

OptString Message::msgcomments() const
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

void Message::editMsgstr(int index, const std::string& str)
{
    assert(index >= 0 && index < numPlurals());
    assert(!m_msgstr[index].isNull());

    if (m_msgstr[index] == str)
        return;

    m_msgstr[index] = str;
    m_edited = true;
}

void Message::editMsgcomments(const OptString& str)
{
    if (m_msgcomments == str)
        return;

    m_msgcomments = str;
    m_edited = true;
}

bool Message::isEdited() const
{
    return m_edited;
}

// Returns whether msgstr[*] and translator's comments are equal in two messages.
// States of 'fuzzy' flag should also be the same.
bool Message::equalTranslationsComments(const Message* o) const
{
    assert(m_plural == o->isPlural());

    return MessageTranslationBase::equalTranslations(o) && m_msgcomments == o->msgcomments();
}
