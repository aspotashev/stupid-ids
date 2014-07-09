#include "messagetranslationbase.h"

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

bool MessageTranslationBase::setNPluralsPacked(int n_plurals)
{
    assert(n_plurals >= 0);

    // n_plurals=0 means that there is only one form "msgstr"
    m_numPlurals = std::max(n_plurals, 1);
    assert(m_numPlurals <= MAX_PLURAL_FORMS); // limited by the size of m_msgstr

    // Return whether the message uses plural forms
    return n_plurals > 0;
}

std::string MessageTranslationBase::formatPoMessage(po_message_t message)
{
    MessageTranslationBase *msg = new MessageTranslationBase(message);
    std::string res = msg->formatPoMessage();
    delete msg;

    return res;
}

std::string MessageTranslationBase::formatPoMessage() const
{
    assert(numPlurals() > 0);

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
            printf("Unescaped special symbol: code = %d\nstr = %s\n", (int)str[i], str);
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

bool MessageTranslationBase::equalTranslations(const MessageTranslationBase *o) const
{
    return equalMsgstr(o) && m_fuzzy == o->isFuzzy();
}

bool MessageTranslationBase::isUntranslated() const
{
    for (int i = 0; i < numPlurals(); i ++)
        if (strlen(msgstr(i)) > 0)
            return false;

    return true;
}
