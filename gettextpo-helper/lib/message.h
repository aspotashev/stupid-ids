
#ifndef MESSAGE_H
#define MESSAGE_H

#include <vector>
#include <assert.h>
#include <string.h>

#include <gettext-po.h>

#include <gettextpo-helper/gettextpo-helper.h>


class IddiffMessage;

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
	~MessageTranslationBase();

	/**
	 * @brief Escape special symbols and put in quotes.
	 *
	 * Escape the following characters: double quote ("), newline, tab, backslash.
	 *
	 * @param str Input string to escape.
	 * @return Escaped string.
	 **/
	static std::string formatString(const char *str);

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
	 * @param plural_form Index of translation form.
	 * @return Pointer to buffer.
	 **/
	const char *msgstr(int plural_form) const;

	void setMsgstr(int index, const char *str);
	int numPlurals() const;
	bool isFuzzy() const;
	bool equalMsgstr(const MessageTranslationBase *o) const;
	bool equalTranslations(const MessageTranslationBase *o) const;

protected:
	/**
	 * @brief Initialize an empty message (without any translations).
	 **/
	virtual void clear();

	bool setNPluralsPacked(int n_plurals);

	static std::string formatPoMessage(po_message_t message);

protected:
	const static int MAX_PLURAL_FORMS = 4; // increase this if you need more plural forms
	int m_numPlurals; // =1 if the message does not use plural forms
	char *m_msgstr[MAX_PLURAL_FORMS];
	bool m_fuzzy;
};

class Message : public MessageTranslationBase
{
public:
	Message(const Message &message);
	Message(po_message_t message, int index, const char *filename);
	Message(bool fuzzy, const char *msgcomment, const char *msgstr0, int n_plurals = 0);
//	Message(bool fuzzy, const char *msgcomment, const char *msgstr0, const char *msgstr1, const char *msgstr2, const char *msgstr3);
	Message(bool fuzzy, int n_plurals, const char *msgcomment);
	~Message();

	int index() const;
	const char *filename() const;
	bool equalTranslationsComments(const Message *o) const;

	bool isPlural() const;
	bool isUntranslated() const;

	/**
	 * @brief Returns whether not untranslated and not fuzzy.
	 **/
	bool isTranslated() const;

	const char *msgcomments() const;

	// Also sets "m_edited" to true.
	// Used for patching translation files.
	void editFuzzy(bool fuzzy);
	void editMsgstr(int index, const char *str);
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

	void addMessage(Message *message);
	int size() const;
	Message *message(int index);
	const Message *message(int index) const;
	void mergeMessageGroup(const MessageGroup *other);

	const char *msgid() const;
	const char *msgidPlural() const;
	const char *msgctxt() const;

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

#endif

