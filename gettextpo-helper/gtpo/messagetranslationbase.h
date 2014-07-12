#ifndef MESSAGETRANSLATIONBASE_H
#define MESSAGETRANSLATIONBASE_H

#include <gtpo/optstring.h>

#include <gettext-po.h>

#include <vector>

/**
 * @brief Base class for classes that contain message translation (classes IddiffMessage, Message).
 **/
class MessageTranslationBase
{
public:
    /**
     * @brief Constructs an empty message (without any translations).
     **/
    MessageTranslationBase();

    /**
     * @brief Constructs a message based on a libgettextpo message.
     *
     * @param message libgettextpo message.
     **/
    MessageTranslationBase(po_message_t message);

    /**
     * @brief Frees memory allocated for message translations.
     *
     **/
    virtual ~MessageTranslationBase();

    /**
     * @brief Escape special symbols and put in quotes.
     *
     * Escape the following characters: double quote ("), newline, tab, backslash.
     *
     * @param str Input string to escape.
     * @return Escaped string.
     **/
    static std::string formatString(const std::string& str);

    /**
     * @brief Format message translations as [f]"form1"[ "form2" ... "formN"]
     *
     * Special characters like "\t" and "\n" will be escaped.
     *
     * @return Formatted message (for .iddiff files, etc).
     **/
    std::string formatPoMessage() const;

    /**
     * @brief Returns a const pointer to a particular translation form.
     *
     * @param index Index of translation form.
     * @return Pointer to buffer.
     **/
    OptString msgstr(size_t index) const;

    /**
     * @brief Sets the translation in one of the forms.
     *
     * @param index Index of plural form.
     * @param str Translation text, will be copied to a newly allocated buffer.
     **/
    void setMsgstr(size_t index, const OptString& str);

    /**
     * @brief Returns the number of plural forms in translation.
     *
     * @return Number of plural forms in translation.
     * Equals 1 if the string does not use plural forms.
     **/
    int numPlurals() const;

    /**
     * @brief Returns whether the message uses plural forms.
     **/
    bool isPlural() const;

    /**
     * @brief Returns whether message has the "fuzzy" flag.
     *
     * @return State of the "fuzzy" flag.
     **/
    bool isFuzzy() const;

    /**
     * @brief Returns whether all translations (and their count)
     * in two messages are identical.
     *
     * @param o Other message.
     * @return Whether msgstr[*] are equal in two messages.
     **/
    bool equalMsgstr(const MessageTranslationBase* o) const;

    /**
     * @brief Returns whether all translations and state of
     * the fuzzy flag in two messages are identical.
     *
     * @param o Other message.
     * @return Whether msgstr[*] and "fuzzy" flag are equal in two messages.
     **/
    bool equalTranslations(const MessageTranslationBase *o) const;

    virtual bool isUntranslated() const;

protected:
    /**
     * @brief Initialize an empty message (without any translations).
     **/
//     virtual void clear();

    /**
     * @brief Initializes m_numPlurals given the value returned from po_message_n_plurals().
     *
     * @param n_plurals Return value from po_message_n_plurals().
     * "packed" means that 'n_plurals' contains more information than m_numPlurals:\n
     * 1. whether the message uses plural forms (n_plurals=0 means that message does not use plural forms)\n
     * 2. the number of plural forms
     *
     * @return Whether the message uses plural forms.
     **/
//     bool setNPluralsPacked(int n_plurals);

    static std::string formatPoMessage(po_message_t message);

protected:
    // Arabic uses 6 plural forms, TBD: grow this at run-time
//     const static int MAX_PLURAL_FORMS = 6;
//     int m_numPlurals; // =1 if the message does not use plural forms
    bool m_plural;
    bool m_fuzzy;
    std::vector<OptString> m_msgstr;
};

#endif // MESSAGETRANSLATIONBASE_H
