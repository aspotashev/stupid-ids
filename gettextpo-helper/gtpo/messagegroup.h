#ifndef MESSAGEGROUP_H
#define MESSAGEGROUP_H

#include "messageoriginaltext.h"

#include <gtpo/optstring.h>

#include <gettext-po.h>

#include <string>
#include <vector>

class Message;

class MessageGroup : public MessageOriginalText
{
public:
    MessageGroup();
    MessageGroup(const MessageGroup& o);
    MessageGroup(po_message_t message, int index, const std::string& filename);
    ~MessageGroup();

    /**
     * @brief Returns the number of translations in the group.
     **/
    int size() const;

    Message *message(int index);
    const Message *message(int index) const;

    bool equalOrigText(const MessageGroup& that) const;
    bool compareByMsgid(const MessageGroup& that) const;

    void updateTranslationFrom(const MessageGroup *from);

    // Makes the message untranslated
    void clearTranslation();

    void mergeMessageGroup(const MessageGroup* other);

protected:
//     void clear();

    /**
     * @brief Add message translation.
     *
     * @param message Pointer to message to be added.
     * This object will be taken ownership of.
     * @return void
     **/
    void addMessage(Message* message);

private:
    std::vector<Message*> m_messages;
};

#endif // MESSAGEGROUP_H
