#include "messageoriginaltext.h"

#include <cassert>

MessageOriginalText::MessageOriginalText()
    : m_msgid(nullptr)
    , m_msgidPlural(nullptr)
    , m_msgctxt(nullptr)
{
}

MessageOriginalText::~MessageOriginalText()
{
}

MessageOriginalText::MessageOriginalText(const MessageOriginalText& o)
    : m_msgid(o.m_msgid)
    , m_msgidPlural(o.m_msgidPlural)
    , m_msgctxt(o.m_msgctxt)
{
}

MessageOriginalText::MessageOriginalText(po_message_t message)
    : m_msgid(po_message_msgid(message))
    , m_msgidPlural(po_message_msgid_plural(message))
    , m_msgctxt(po_message_msgctxt(message))
{
}

void MessageOriginalText::setMsgid(const std::string& str)
{
    assert(m_msgid.isNull());

    m_msgid = OptString(str);
}

void MessageOriginalText::setMsgidPlural(const std::string& str)
{
    assert(m_msgidPlural.isNull());

    m_msgidPlural = OptString(str);
}

void MessageOriginalText::setMsgctxt(const std::string& str)
{
    assert(m_msgctxt.isNull());

    m_msgctxt = OptString(str);
}

OptString MessageOriginalText::msgid() const
{
    return m_msgid;
}

OptString MessageOriginalText::msgidPlural() const
{
    // Checking that m_msgid is initialized. This should mean
    // that m_msgidPlural is also set (probably, set to NULL).
    assert(!m_msgid.isNull());

    return m_msgidPlural;
}

OptString MessageOriginalText::msgctxt() const
{
    // Checking that m_msgid is initialized. This should mean
    // that m_msgctxt is also set (probably, set to NULL).
    assert(!m_msgid.isNull());

    return m_msgctxt;
}

bool MessageOriginalText::operator==(const MessageOriginalText& that) const
{
    return m_msgid == that.m_msgid &&
        m_msgidPlural == that.m_msgidPlural &&
        m_msgctxt == that.m_msgctxt;
}
