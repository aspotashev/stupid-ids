
// http://www.gnu.org/software/gettext/manual/gettext.html#libgettextpo

#include <string>
#include <vector>
#include <map>
#include <assert.h>
#include <gettext-po.h>

void xerror_handler(
	int severity,
	po_message_t message, const char *filename, size_t lineno,
	size_t column, int multiline_p, const char *message_text)
{
	printf("filename = %s, lineno = %lu, column = %lu\n", filename, lineno, column);
//	assert(0);
}

void xerror2_handler(
	int severity,
	po_message_t message1, const char *filename1, size_t lineno1,
	size_t column1, int multiline_p1, const char *message_text1,
	po_message_t message2, const char *filename2, size_t lineno2,
	size_t column2, int multiline_p2, const char *message_text2)
{
	printf("filename1 = %s, lineno1 = %lu, column1 = %lu, message_text1 = %s\n", filename1, lineno1, column1, message_text1);
	printf("filename2 = %s, lineno2 = %lu, column2 = %lu, message_text2 = %s\n", filename2, lineno2, column2, message_text2);
//	assert(0);
}

// overloaded function, without 'xerror_handlers' argument
po_file_t po_file_read(const char *filename)
{
	struct po_xerror_handler xerror_handlers;
	xerror_handlers.xerror = xerror_handler;
	xerror_handlers.xerror2 = xerror2_handler;

	po_file_t file = po_file_read(filename, &xerror_handlers);
	if (file == NULL)
	{
		printf("Cannot read .po/.pot file: %s\n", filename);
		assert(0);
	}

	// ensure that there is only one domain "messages"
	const char * const *domains = po_file_domains(file);
	assert(strcmp(domains[0], "messages") == 0); // domain is not called "messages"
	assert(domains[1] == NULL); // more than one domain

	return file;
}

po_file_t po_buffer_read(const char *buffer, size_t length)
{
	// Child process writes the contents of the buffer to a pipe.
	// The read end of the pipe is connected to 'stdin' (for 'po_file_read').

	int pipe_fd[2];
	assert(pipe(pipe_fd) == 0);

	int stdin_fd = dup(0); // preserve 'stdin'
	assert(stdin_fd >= 0);
	close(0);

	assert(dup2(pipe_fd[0], 0) >= 0); // connect pipe's read end to stdin
	close(pipe_fd[0]);

	// pipe_fd[1] -- write end
	// fd=0 -- read end
	pid_t pid = fork();
	if (pid < 0) // fork failed
	{
		assert(0);
	}
	else if (pid == 0) // child process
	{
		close(0);

		// TODO: report errors from "write()" to the parent process (using a buffer + semaphore?)
		write(pipe_fd[1], buffer, length);

		exit(0); // terminate child process
	}

	// In parent process now

	close(pipe_fd[1]);
	po_file_t file = po_file_read("-"); // read from stdin

	// Restore 'stdin'
	close(0);
	assert(dup2(stdin_fd, 0) >= 0);

	return file;
}

//----------------------- Dumping translation files ------------------------

// Transform string into a string of characters [0-9a-f] or "n" if str is NULL.
std::string wrap_string_hex(const char *str)
{
	if (str == NULL)
		return "n";

	std::string res;

	size_t len = strlen(str);
	char hex[3];
	for (size_t i = 0; i < len; i ++)
	{
		sprintf(hex, "%02x", (unsigned char)str[i]);
		res += hex;
	}

	return res;
}

//----------------------- Calculation of template-part hash ------------------------

#if !defined (QT_VERSION)
	#define SHA1_USING_CRYPTOPP
#endif

#if defined (SHA1_USING_CRYPTOPP)

#include <cryptopp/sha.h> // CryptoPP::SHA1
#include <cryptopp/filters.h> // CryptoPP::StringSource
#include <cryptopp/hex.h> // CryptoPP::HexEncoder

// http://www.xenoterracide.com/2009/09/quick-sha1sum-with-crypto.html
// http://groups.google.com/group/cryptopp-users/browse_thread/thread/dfe40b4eed04f03d?pli=1
std::string sha1_string(std::string input)
{
	std::string res;

	CryptoPP::SHA1 hash;
	CryptoPP::StringSink *string_sink = new CryptoPP::StringSink(res);
	CryptoPP::HexEncoder *hex_encoder = new CryptoPP::HexEncoder(string_sink, false); // hex in lowercase
	CryptoPP::HashFilter *hash_filter = new CryptoPP::HashFilter(hash, hex_encoder);
	CryptoPP::StringSource(input, true, hash_filter);

	return res;
}

#else

#include <QCryptographicHash>

// sha1 using Qt
// http://www.qtcentre.org/threads/16846-How-to-get-sha1-hash

std::string sha1_string(std::string input)
{
	QCryptographicHash hash(QCryptographicHash::Sha1);
	hash.addData(input.c_str(), (int)input.length());
	return std::string(hash.result().toHex().data());
}

#endif

std::string wrap_template_header(po_message_t message)
{
	char *header = new char [strlen(po_message_msgstr(message)) + 2];
	header[0] = '\n'; // prepend "\n" to simplify search for "\nPOT-Creation-Date: "
	strcpy(header + 1, po_message_msgstr(message));

	// find the "POT-Creation-Date:" field
	const char *pot_creation_pattern = "\nPOT-Creation-Date: "; // text that goes before the POT creation date
	char *pot_creation_str = strstr(header, pot_creation_pattern);
	assert(pot_creation_str != NULL); // There must be a "POT-Creation-Date:" in the header.

	// extract the date
	pot_creation_str += strlen(pot_creation_pattern); // move to the beginning of the date
	char *pot_date_end = strchr(pot_creation_str, '\n');
	assert(pot_date_end != NULL); // There must be a "\n" after every line (including "POT-Creation-Date: ...") in the header.
	*pot_date_end = '\0'; // truncate the string


	std::string res = wrap_string_hex(pot_creation_str);
	delete [] header;

	return res;
}

std::string dump_filepos_entries(po_message_t message)
{
	std::string res;

	po_filepos_t filepos;
	char filepos_start_line[20];
	// FIXME: "filepos" entries should probably be sorted alphabetically
	// (because PO editors might reorder them).
	for (int i = 0; filepos = po_message_filepos(message, i); i ++)
	{
		res += po_filepos_file(filepos);
		res += '\n';

		// Convert "po_filepos_start_line" to "int", because the return value is
		// (size_t)(-1) when no line number is available.
		sprintf(filepos_start_line, "%d", (int)po_filepos_start_line(filepos));
		res += filepos_start_line;
		res += '\n';
	}

	return res;
}

std::string dump_format_types(po_message_t message)
{
	std::string res;

	// FIXME: "*-format" entries should probably be sorted alphabetically
	// (because they might be reordered in other versions of "libgettext-po").
	for (int i = 0; po_format_list()[i] != NULL; i ++)
		if (po_message_is_format(message, po_format_list()[i]))
		{
			res += po_format_list()[i];
			res += '\n';
		}

	return res;
}

// include_non_id: include 'extracted comments', 'filepos entries', 'format types', 'range'
std::string wrap_template_message(po_message_t message, bool include_non_id)
{
	// header should be processed separately
	assert(*po_message_msgid(message) != '\0' || po_message_msgctxt(message) != NULL);

	if (po_message_is_obsolete(message)) // obsolete messages should not affect the dump
		return std::string();


	std::string res;

	res += "T"; // "T". May be NULL.
	res += wrap_string_hex(po_message_msgctxt(message));

	res += "M"; // "M". Cannot be NULL or empty.
	res += wrap_string_hex(po_message_msgid(message));

	res += "P"; // "P". May be NULL.
	res += wrap_string_hex(po_message_msgid_plural(message));

	if (!include_non_id)
		return res; // short dump used by "dump-ids"

	// additional fields (for calculation of "template-part hash")
	res += "C"; // "C". Cannot be NULL, may be empty.
	res += wrap_string_hex(po_message_extracted_comments(message));

	res += "N";
	res += wrap_string_hex(dump_filepos_entries(message).c_str());

	res += "F";
	res += wrap_string_hex(dump_format_types(message).c_str());

	int minp, maxp;
	if (po_message_is_range(message, &minp, &maxp)) // I have never seen POTs with ranges, but anyway...
	{
		char range_dump[30];
		sprintf(range_dump, "%xr%x", minp, maxp); // not characters, but integers are encoded in HEX here

		res += "R";
		res += range_dump;
	}


	return res;
}

// Pass filename = "-" to read .po file from stdin.
std::string dump_po_file_template(const char *filename)
{
	std::string res;

	po_file_t file = po_file_read(filename); // all checks and error reporting are done in po_file_read


	// main cycle
	po_message_iterator_t iterator = po_message_iterator(file, "messages");
	po_message_t message; // in fact, this is a pointer

	// processing header (header is the first message)
	message = po_next_message(iterator);
	res += wrap_template_header(message);

	// ordinary .po messages (not header)
	//
	// Assuming that PO editors do not change the order of messages.
	// Sorting messages in alphabetical order would be wrong, because for every template,
	// we store only the ID of the first message. The IDs of other messages should be deterministic.
	while (message = po_next_message(iterator))
	{
		std::string msg_dump = wrap_template_message(message, true);
		if (msg_dump.length() > 0) // non-obsolete
		{
			res += "/";
			res += msg_dump;
		}
	}

	po_file_free(file); // free memory
	return res;
}

std::string calculate_tp_hash(const char *filename)
{
	return sha1_string(dump_po_file_template(filename));
}

// Returns the number of messages in .pot (excluding the header)
int get_pot_length(const char *filename)
{
	po_file_t file = po_file_read(filename); // all checks and error reporting are done in po_file_read

	po_message_iterator_t iterator = po_message_iterator(file, "messages");
	po_message_t message; // in fact, this is a pointer

	// processing header (header is the first message)
	message = po_next_message(iterator);
	assert(*po_message_msgid(message) == '\0'); // msgid of header must be empty

	int length = 0;
	// ordinary .po messages (not header)
	while (message = po_next_message(iterator))
		if (!po_message_is_obsolete(message)) // in fact, if we're processing a .pot, there can't be any obsolete messages
			length ++;

	po_file_free(file); // free memory
	return length;
}

//-------- Coupling IDs of equal messages in different .po/.pot files -------

std::vector<std::pair<std::string, int> > dump_po_file_ids(const char *filename, int first_id)
{
	std::vector<std::pair<std::string, int> > res;

	po_file_t file = po_file_read(filename); // all checks and error reporting are done in po_file_read

	// main cycle
	po_message_iterator_t iterator = po_message_iterator(file, "messages");
	po_message_t message; // in fact, this is a pointer

	// skipping header
	message = po_next_message(iterator);

	for (int index = 0; message = po_next_message(iterator); index ++)
	{
		std::string msg_dump = wrap_template_message(message, false);
		if (msg_dump.length() > 0)
			res.push_back(make_pair(msg_dump, first_id + index));
	}

	po_file_free(file); // free memory
	return res;
}

std::vector<std::vector<int> > list_equal_messages_ids(std::vector<std::pair<const char *, int> > files)
{
	std::vector<std::vector<int> > list;

	typedef std::map<std::string, std::vector<int> > msg_ids_map_t;
	msg_ids_map_t msg_ids;
	for (size_t d = 0; d < files.size(); d ++)
	{
		std::vector<std::pair<std::string, int> > dump = dump_po_file_ids(files[d].first, files[d].second);
		for (size_t i = 0; i < dump.size(); i ++)
			msg_ids[dump[i].first].push_back(dump[i].second);
	}

	for (msg_ids_map_t::iterator iter = msg_ids.begin(); iter != msg_ids.end(); iter ++)
	{
		size_t n_ids = iter->second.size();
		if (n_ids == 0) // impossible
		{
			assert(0);
		}
		else if (n_ids >= 2)
		{
			// This 'assert' can be removed if files.size() > 2 (not doing this now,
			// because I do not know where a 3-way dump-ids may be applied).
			assert(n_ids == 2);

			list.push_back(iter->second); // adding a vector of all IDs of this message
		}
	}

	return list;
}

std::vector<std::pair<int, int> > list_equal_messages_ids_2(const char *filename_a, int first_id_a, const char *filename_b, int first_id_b)
{
	std::vector<std::pair<const char *, int> > files;
	files.push_back(std::pair<const char *, int>(filename_a, first_id_a));
	files.push_back(std::pair<const char *, int>(filename_b, first_id_b));

	std::vector<std::vector<int> > list = list_equal_messages_ids(files);

	std::vector<std::pair<int, int> > res;
	for (std::vector<std::vector<int> >::iterator iter = list.begin(); iter != list.end(); iter ++)
	{
		// No more than 2 IDs denoting the same message can exists in 2 files
		assert(iter->size() == 2);

		res.push_back(std::pair<int, int>((*iter)[0], (*iter)[1]));
	}

	return res;
}

//------ For diff'ing tools ------

// Returns 'true' when msgstr (or all msgstr[i]) are empty.
bool po_message_is_untranslated(po_message_t message)
{
	if (po_message_msgstr_plural(message, 0)) // message with plurals
	{
		for (int i = 0; po_message_msgstr_plural(message, i); i ++)
			if (po_message_msgstr_plural(message, i)[0] != '\0') // non-empty string found
				return false; // not untranslated (i.e. translated or fuzzy)

		return true; // all strings are empty
	}
	else // message without plurals
	{
		return po_message_msgstr(message)[0] == '\0';
	}
}

// Returns 0 when the message does not use plural forms (i.e. no "msgid_plural")
int po_message_n_plurals(po_message_t message)
{
	int i;
	for (i = 0; po_message_msgstr_plural(message, i); i ++);

	return i;
}

// Returns 0 when msgstr (or all msgstr[i]) are the same in two messages.
int compare_po_message_msgstr(po_message_t message_a, po_message_t message_b)
{
	int n_plurals = po_message_n_plurals(message_a);
	assert(n_plurals == po_message_n_plurals(message_b));

	if (n_plurals > 0) // messages with plurals
	{
		for (int i = 0; i < n_plurals; i ++)
			if (strcmp(
				po_message_msgstr_plural(message_a, i),
				po_message_msgstr_plural(message_b, i)))
				return 1; // differences found

		return 0; // all strings are equal
	}
	else // message without plurals
	{
		return strcmp(po_message_msgstr(message_a), po_message_msgstr(message_b));
	}
}

//------ C++ wrapper library for 'libgettextpo' with some extra features -------

class Message // TODO: Ruby bindings
{
public:
	Message(po_message_t message, int index, const char *filename);
	~Message();

	int index() const;
	const char *filename() const;
	bool equalTranslations(const Message *o) const;

	bool isFuzzy() const
	{
		return m_fuzzy;
	}

	bool isPlural() const
	{
		return m_plural;
	}

	int numPlurals() const
	{
		return m_numPlurals;
	}

	const char *msgstr(int plural_form) const
	{
		assert(plural_form >= 0 && plural_form < m_numPlurals);

		return m_msgstr[plural_form];
	}

	const char *msgcomments() const
	{
		return m_msgcomments;
	}

private:
//	char *m_msgid;
//	char *m_msgidPlural;

	const static int MAX_PLURAL_FORMS = 4; // increase this if you need more plural forms

	bool m_plural; // =true if message uses plural forms
	int m_numPlurals; // =1 if the message does not use plural forms
	char *m_msgstr[MAX_PLURAL_FORMS];
	char *m_msgcomments;
	bool m_fuzzy;

	int m_index;
	const char *m_filename;
};

Message::Message(po_message_t message, int index, const char *filename):
	m_index(index),
	m_filename(filename)
{
	m_numPlurals = po_message_n_plurals(message);
	if (m_numPlurals == 0) // message does not use plural forms
	{
		m_numPlurals = 1;
		m_plural = false;
	}
	else
	{
		m_plural = true;
	}

	assert(m_numPlurals <= MAX_PLURAL_FORMS); // limited by the size of m_msgstr

	for (int i = 0; i < m_numPlurals; i ++)
	{
		const char *tmp;
		if (m_plural)
		{
			tmp = po_message_msgstr_plural(message, i);
		}
		else
		{
			assert(i == 0); // there can be only one form if 'm_plural' is false

			tmp = po_message_msgstr(message);
		}

		m_msgstr[i] = new char[strlen(tmp) + 1];
		strcpy(m_msgstr[i], tmp);
	}

	m_fuzzy = po_message_is_fuzzy(message);

	// translators' comments
	const char *tmp = po_message_comments(message);
	m_msgcomments = new char[strlen(tmp) + 1];
	strcpy(m_msgcomments, tmp);
}

Message::~Message()
{
}

int Message::index() const
{
	return m_index;
}

const char *Message::filename() const
{
	return m_filename;
}

// Returns whether msgstr[*] and translator's comments are equal in two messages.
// States of 'fuzzy' flag should also be the same.
bool Message::equalTranslations(const Message *o) const
{
	assert(m_plural == o->isPlural());
	assert(m_numPlurals == o->numPlurals());

	for (int i = 0; i < m_numPlurals; i ++)
		if (strcmp(m_msgstr[i], o->msgstr(i)))
			return false;

	return m_fuzzy == o->isFuzzy() && !strcmp(m_msgcomments, o->msgcomments());
}

