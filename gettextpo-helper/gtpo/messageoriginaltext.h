#ifndef MESSAGEORIGINALTEXT_H
#define MESSAGEORIGINALTEXT_H

#include <gtpo/optstring.h>

#include <gettext-po.h>

#include <string>

class Message;

class MessageOriginalText
{
public:
    MessageOriginalText();
    MessageOriginalText(const MessageOriginalText& o);
    MessageOriginalText(po_message_t message);
    ~MessageOriginalText();

    OptString msgid() const;

    /**
    * \brief Returns NULL for messages without plural forms.
    */
    OptString msgidPlural() const;

    OptString msgctxt() const;

    bool operator==(const MessageOriginalText& that) const;

protected:
    /**
    * Does not take ownership of "str".
    */
    void setMsgid(const std::string& str);

    /**
    * Does not take ownership of "str".
    */
    void setMsgidPlural(const std::string& str);

    /**
    * Does not take ownership of "str".
    */
    void setMsgctxt(const std::string& str);

private:
    OptString m_msgid;
    OptString m_msgidPlural;
    OptString m_msgctxt;
};

#endif // MESSAGEORIGINALTEXT_H
