#include "messagetranslationbase.h"
#include "gettextpo-helper.h"

#include <algorithm>
#include <iostream>

MessageTranslationBase::MessageTranslationBase()
    : m_plural(false)
    , m_fuzzy(false)
    , m_msgstr()
{
}

MessageTranslationBase::MessageTranslationBase(po_message_t message)
{
    // TBD: optimize - calling po_message_n_plurals() is not required
    int n_plurals = po_message_n_plurals(message);

    m_plural = n_plurals > 0;

    if (n_plurals < 1)
        n_plurals = 1;

    m_msgstr.resize(n_plurals, OptString(nullptr));

    for (int i = 0; i < n_plurals; ++i) {
        const char *tmp;
        if (m_plural) {
            tmp = po_message_msgstr_plural(message, i);
        }
        else {
            assert(i == 0); // there can be only one form if 'm_plural' is false

            tmp = po_message_msgstr(message);
        }

        setMsgstr(i, OptString(tmp));
    }

    m_fuzzy = po_message_is_fuzzy(message) != 0;
}

MessageTranslationBase::~MessageTranslationBase()
{
}

// void MessageTranslationBase::clear()
// {
//     m_numPlurals = 0;
//     for (int i = 0; i < MAX_PLURAL_FORMS; i ++)
//         m_msgstr[i] = NULL;
//
//     m_fuzzy = false;
// }
/*
bool MessageTranslationBase::setNPluralsPacked(int n_plurals)
{
    assert(n_plurals >= 0);

    // n_plurals=0 means that there is only one form "msgstr"
    m_numPlurals = std::max(n_plurals, 1);
    assert(m_numPlurals <= MAX_PLURAL_FORMS); // limited by the size of m_msgstr

    // Return whether the message uses plural forms
    return n_plurals > 0;
}*/

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

void MessageTranslationBase::writeJson(rapidjson::PrettyWriter<rapidjson::StringBuffer> &writer) const
{
    assert(numPlurals() > 0);

    writer.StartObject();

    writer.String("fuzzy");
    writer.Bool(isFuzzy());

    writer.String("msgstr");
    writer.StartArray();
    for (int i = 0; i < numPlurals(); i ++) {
        writer.String(msgstr(i).c_str());
    }
    writer.EndArray();

    writer.EndObject();
}

std::string MessageTranslationBase::toJson() const
{
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);

    writeJson(writer);

    return std::string(buffer.GetString());
}

std::string MessageTranslationBase::formatString(const std::string& str)
{
    std::string res;

    res += "\""; // opening quote

    size_t len = str.size();
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
        else if ((unsigned char)str[i] < ' ') {
            std::cout << "Unescaped special symbol: code = " << (int)str[i] << std::endl <<
                "str = " << str << std::endl;
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

OptString MessageTranslationBase::msgstr(size_t index) const
{
    assert(index < m_msgstr.size());

    return m_msgstr[index];
}

void MessageTranslationBase::setMsgstr(size_t index, const OptString& str)
{
    assert(index < m_msgstr.size());
    assert(m_msgstr[index].isNull());

    m_msgstr[index] = str;
}

int MessageTranslationBase::numPlurals() const
{
    return m_msgstr.size();
}

bool MessageTranslationBase::isPlural() const
{
    return m_plural;
}

bool MessageTranslationBase::equalMsgstr(const MessageTranslationBase* o) const
{
    // This happens, see messages/kdebase/plasma_applet_trash.po
    // from http://websvn.kde.org/?view=revision&revision=825044
    if (numPlurals() != o->numPlurals())
        return false;

    return m_msgstr == o->m_msgstr; // TBD: check that this compares each msgstr[i]
}

bool MessageTranslationBase::equalTranslations(const MessageTranslationBase* o) const
{
    return equalMsgstr(o) && m_fuzzy == o->m_fuzzy;
}

bool MessageTranslationBase::isUntranslated() const
{
    return std::all_of(
        m_msgstr.begin(), m_msgstr.end(),
        [](const OptString& s) { return s.empty(); });
}
