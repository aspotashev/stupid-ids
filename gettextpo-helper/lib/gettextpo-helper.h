
// http://www.gnu.org/software/gettext/manual/gettext.html#libgettextpo

#include <string>
#include <vector>
#include <map>
#include <assert.h>
#include <gettext-po.h>

#include <sys/wait.h> // for waitpid

//------------------------------

char *xstrdup(const char *str);

//------------------------------

// Hidden function from libgettextpo.
// TODO: open access to this function (may be libgettextpo should be patched?)
//extern void libgettextpo_message_page_width_set(size_t width);

//------------------------------

// overloaded function, without 'xerror_handlers' argument
po_file_t po_file_read(const char *filename);

// overloaded function, without 'xerror_handlers' argument
po_file_t po_file_write(po_file_t file, const char *filename);

po_file_t po_buffer_read(const char *buffer, size_t length);

//----------------------- Dumping translation files ------------------------

// Transform string into a string of characters [0-9a-f] or "n" if str is NULL.
std::string wrap_string_hex(const char *str);

//----------------------- Calculation of template-part hash ------------------------

std::string sha1_string(std::string input);

std::string wrap_template_header(po_message_t message);
std::string dump_filepos_entries(po_message_t message);
std::string dump_format_types(po_message_t message);

// include_non_id: include 'extracted comments', 'filepos entries', 'format types', 'range'
std::string wrap_template_message(po_message_t message, bool include_non_id);

std::string calculate_tp_hash(const char *filename);

// Returns the number of messages in .pot (excluding the header)
int get_pot_length(const char *filename);

//-------- Coupling IDs of equal messages in different .po/.pot files -------

class MappedFileIdMapDb;
class TranslationContent;

std::vector<std::pair<std::string, int> > dump_po_file_ids(TranslationContent *content, int first_id);

std::vector<std::vector<int> > list_equal_messages_ids(std::vector<std::pair<TranslationContent *, int> > files);

std::vector<std::pair<int, int> > list_equal_messages_ids_2(TranslationContent *file_a, int first_id_a, TranslationContent *file_b, int first_id_b);

int dump_equal_messages_to_mmapdb(TranslationContent *file_a, int first_id_a, TranslationContent *file_b, int first_id_b, MappedFileIdMapDb *mmap_db);

//------ For diff'ing tools ------

// Returns 'true' when msgstr (or all msgstr[i]) are empty.
bool po_message_is_untranslated(po_message_t message);

// Returns 0 when the message does not use plural forms (i.e. no "msgid_plural")
int po_message_n_plurals(po_message_t message);

// Returns 0 when msgstr (or all msgstr[i]) are the same in two messages.
int compare_po_message_msgstr(po_message_t message_a, po_message_t message_b);

//------ C++ wrapper library for 'libgettextpo' with some extra features -------

class Message
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

	bool isFuzzy() const;
	bool isPlural() const;
	bool isUntranslated() const;
	int numPlurals() const;
	const char *msgstr(int plural_form) const;
	const char *msgcomments() const;

	void setMsgstr(int index, const char *str);

	// Also sets "m_edited" to true.
	// Used for patching translation files.
	void editFuzzy(bool fuzzy);
	void editMsgstr(int index, const char *str);
	bool isEdited() const;

protected:
	void setMsgcomments(const char *str);
	void clear();
	void setNPluralsPacked(int n_plurals);

private:
	const static int MAX_PLURAL_FORMS = 4; // increase this if you need more plural forms

	bool m_plural; // =true if message uses plural forms
	int m_numPlurals; // =1 if the message does not use plural forms
	char *m_msgstr[MAX_PLURAL_FORMS];
	char *m_msgcomments;
	bool m_fuzzy;
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

std::vector<MessageGroup *> read_po_file_messages(const char *filename, bool loadObsolete);

