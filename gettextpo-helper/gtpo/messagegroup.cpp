#include "messagegroup.h"
#include "message.h"

#include <tuple>

#include <cassert>

MessageGroup::MessageGroup()
    : MessageOriginalText()
    , m_messages()
{
}

MessageGroup::MessageGroup(const MessageGroup& o)
    : MessageOriginalText(o)
    , m_messages()
{
    for (Message* msg : o.m_messages)
        m_messages.push_back(new Message(*msg));
}

MessageGroup::MessageGroup(po_message_t message, int index, const std::string& filename)
    : MessageOriginalText(message)
{
    addMessage(new Message(message, index, filename));
}

MessageGroup::~MessageGroup()
{
//     for (size_t i = 0; i < m_messages.size(); i ++)
//         delete m_messages[i];
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

void MessageGroup::clearTranslation()
{
    assert(size() == 1);

    m_messages[0]->clearTranslation();
}
