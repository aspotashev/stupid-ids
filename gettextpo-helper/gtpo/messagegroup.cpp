#include "messagegroup.h"
#include "message.h"

#include <tuple>

#include <cassert>

MessageGroup::MessageGroup()
    : m_msgid(nullptr)
    , m_msgidPlural(nullptr)
    , m_msgctxt(nullptr)
    , m_messages()
{
}

MessageGroup::MessageGroup(po_message_t message, int index, const std::string& filename)
    : m_msgid(po_message_msgid(message))
    , m_msgidPlural(po_message_msgid_plural(message))
    , m_msgctxt(po_message_msgctxt(message))
{
    addMessage(new Message(message, index, filename));
}

MessageGroup::~MessageGroup()
{
    for (size_t i = 0; i < m_messages.size(); i ++)
        delete m_messages[i];
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
//  TODO:
//  assert(!strcmp(msgid(), other->msgid()));
//  ...

    for (int i = 0; i < other->size(); i ++)
        addMessage(new Message(*other->message(i)));
}

// void MessageGroup::clear()
// {
//     m_msgid.setNull();
//     m_msgidPlural = NULL;
//     m_msgctxt = NULL;
// }

/**
 * Does not take ownership of "str".
 */
void MessageGroup::setMsgid(const std::string& str)
{
    assert(m_msgid.isNull());

    m_msgid = OptString(str);
}

/**
 * Does not take ownership of "str".
 */
void MessageGroup::setMsgidPlural(const std::string& str)
{
    assert(m_msgidPlural.isNull());

    m_msgidPlural = OptString(str);
}

/**
 * Does not take ownership of "str".
 */
void MessageGroup::setMsgctxt(const std::string& str)
{
    assert(m_msgctxt.isNull());

    m_msgctxt = OptString(str);
}

OptString MessageGroup::msgid() const
{
    return m_msgid;
}

/**
 * \brief Returns NULL for messages without plural forms.
 */
OptString MessageGroup::msgidPlural() const
{
    // Checking that m_msgid is initialized. This should mean
    // that m_msgidPlural is also set (probably, set to NULL).
    assert(!m_msgid.isNull());

    return m_msgidPlural;
}

OptString MessageGroup::msgctxt() const
{
    // Checking that m_msgid is initialized. This should mean
    // that m_msgctxt is also set (probably, set to NULL).
    assert(!m_msgid.isNull());

    return m_msgctxt;
}

bool MessageGroup::equalOrigText(const MessageGroup* other) const
{
    return msgid() == other->msgid() &&
        msgctxt() == other->msgctxt() &&
        msgidPlural() == other->msgidPlural();
}

void MessageGroup::updateTranslationFrom(const MessageGroup* from)
{
    assert(size() == 1);
    assert(from->size() == 1);

    delete m_messages[0];
    m_messages[0] = new Message(*(from->message(0)));
}

bool MessageGroup::compareByMsgid(const MessageGroup& o) const
{
    std::tuple<OptString, OptString, OptString> tupleA(
        msgctxt(), msgid(), msgidPlural());
    std::tuple<OptString, OptString, OptString> tupleB(
        o.msgctxt(), o.msgid(), o.msgidPlural());

    return tupleA < tupleB;
}
