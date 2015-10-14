#ifndef MESSAGEGROUP_H
#define MESSAGEGROUP_H

#include <gtpo/optstring.h>

#include <gettext-po.h>

#include <string>
#include <vector>

class Message;

class MessageGroup
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

    OptString msgid() const;
    OptString msgidPlural() const;
    OptString msgctxt() const;

    bool equalOrigText(const MessageGroup *other) const;

    void updateTranslationFrom(const MessageGroup *from);

    bool compareByMsgid(const MessageGroup& o) const;

    // Makes the message untranslated
    void clearTranslation();

    void mergeMessageGroup(const MessageGroup* other);

protected:
//     void clear();
    void setMsgid(const std::string& str);
    void setMsgidPlural(const std::string& str);
    void setMsgctxt(const std::string& str);

    /**
     * @brief Add message translation.
     *
     * @param message Pointer to message to be added.
     * This object will be taken ownership of.
     * @return void
     **/
    void addMessage(Message* message);

private:
    OptString m_msgid;
    OptString m_msgidPlural;
    OptString m_msgctxt;

    std::vector<Message*> m_messages;
};

#endif // MESSAGEGROUP_H
