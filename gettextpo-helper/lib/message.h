
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
	MessageTranslationBase();
	MessageTranslationBase(po_message_t message);

	static std::string formatString(const char *str);
	std::string formatPoMessage() const;
	const char *msgstr(int plural_form) const;
	void setMsgstr(int index, const char *str);
	int numPlurals() const;
	bool isFuzzy() const;
	bool equalMsgstr(const MessageTranslationBase *o) const;
	bool equalTranslations(const MessageTranslationBase *o) const;

protected:
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
	Message(po_message_t message, int index, const char *filename);
	Message(bool fuzzy, const char *msgcomment, const char *msgstr0, int n_plurals = 0);
//	Message(bool fuzzy, const char *msgcomment, const char *msgstr0, const char *msgstr1, const char *msgstr2, const char *msgstr3);
	Message(bool fuzzy, int n_plurals, const char *msgcomment);
	~Message();

	int index() const;
	const char *filename() const;
	bool equalTranslations(const Message *o) const;

	bool isPlural() const;
	bool isUntranslated() const;
	bool isTranslated() const;
	const char *msgcomments() const;

	// Also sets "m_edited" to true.
	// Used for patching translation files.
	void editFuzzy(bool fuzzy);
	void editMsgstr(int index, const char *str);
	bool isEdited() const;

protected:
	void setMsgcomments(const char *str);
	void clear();

private:
	bool m_plural; // =true if message uses plural forms
	char *m_msgcomments;
	bool m_obsolete;
	bool m_untranslated;

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
	void mergeMessageGroup(MessageGroup *other);

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

