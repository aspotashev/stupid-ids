#ifndef MESSAGE_H
#define MESSAGE_H

#include <vector>
#include <assert.h>
#include <string.h>

#include <gettext-po.h>

#include <gtpo/gettextpo-helper.h>
#include <gtpo/messagetranslationbase.h>

class IddiffMessage;

class Message : public MessageTranslationBase
{
public:
    /**
     * @brief Copy constructor.
     *
     * @param message Message to copy from.
     **/
    Message(const Message &message);

    Message(po_message_t message, int index, const char *filename);
    Message(bool fuzzy, const char *msgcomment, const char *msgstr0, int n_plurals = 0);
    Message(bool fuzzy, int n_plurals, const char *msgcomment);
    ~Message();

    int index() const;
    const char *filename() const;
    bool equalTranslationsComments(const Message *o) const;

    /**
     * @brief Returns whether the message uses plural forms.
     **/
    bool isPlural() const;

    virtual bool isUntranslated() const;

    /**
     * @brief Returns whether not untranslated and not fuzzy.
     **/
    bool isTranslated() const;

    const char *msgcomments() const;

    // Also sets "m_edited" to true.
    // Used for patching translation files.
    void editFuzzy(bool fuzzy);
    void editMsgstr(int index, const char *str);
    void editMsgcomments(const char *str);
    bool isEdited() const;

protected:
    void setMsgcomments(const char *str);
    virtual void clear();

private:
    bool m_plural; // =true if message uses plural forms
    bool m_obsolete;
    bool m_untranslated;
    char *m_msgcomments;

    int m_index;
    const char *m_filename;

    bool m_edited;
};

class MessageGroup
{
public:
    MessageGroup();
    MessageGroup(po_message_t message, int index, const char *filename);
    ~MessageGroup();

    /**
     * @brief Add message translation.
     *
     * @param message Pointer to message to be added.
     * This object will be taken ownership of.
     * @return void
     **/
    void addMessage(Message *message);

    /**
     * @brief Returns the number of translations in the group.
     **/
    int size() const;

    Message *message(int index);
    const Message *message(int index) const;
    void mergeMessageGroup(const MessageGroup *other);

    const char *msgid() const;
    const char *msgidPlural() const;
    const char *msgctxt() const;

    bool equalOrigText(const MessageGroup *other) const;

    void updateTranslationFrom(const MessageGroup *from);

protected:
    void clear();
    void setMsgid(const char *str);
    void setMsgidPlural(const char *str);
    void setMsgctxt(const char *str);

private:
    char *m_msgid;
    char *m_msgidPlural;
    char *m_msgctxt;

    std::vector<Message *> m_messages;
};

#endif // MESSAGE_H
