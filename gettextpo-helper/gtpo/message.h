#ifndef MESSAGE_H
#define MESSAGE_H

#include <gtpo/gettextpo-helper.h>
#include <gtpo/messagetranslationbase.h>
#include <gtpo/optstring.h>

#include <gettext-po.h>

#include <vector>

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

    Message(po_message_t message, int index, const std::string& filename);
    Message(bool fuzzy,
            const std::string& msgcomment,
            const std::string& msgstr0, int n_plurals = 0);
    Message(bool fuzzy,
            int n_plurals,
            const std::string& msgcomment);
    ~Message();

    int index() const;
    std::string filename() const;
    bool equalTranslationsComments(const Message *o) const;

    virtual bool isUntranslated() const;

    /**
     * @brief Returns whether not untranslated and not fuzzy.
     **/
    bool isTranslated() const;

    OptString msgcomments() const;

    // Also sets "m_edited" to true.
    // Used for patching translation files.
    void editFuzzy(bool fuzzy);
    void editMsgstr(int index, const std::string& str);
    void editMsgcomments(const OptString& str);
    bool isEdited() const;

protected:
    void setMsgcomments(const std::string& str);
//     virtual void clear();

private:
//     bool m_plural; // =true if message uses plural forms
    bool m_obsolete;
    bool m_untranslated;
    OptString m_msgcomments;

    int m_index;
    std::string m_filename;

    bool m_edited;
};

#endif // MESSAGE_H
