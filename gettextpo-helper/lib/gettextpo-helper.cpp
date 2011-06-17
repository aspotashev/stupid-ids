
#include <stdio.h>
#include <string.h>

#include <gettextpo-helper/gettextpo-helper.h>
#include <gettextpo-helper/mappedfile.h>
#include <gettextpo-helper/translationcontent.h>
#include <gettextpo-helper/iddiff.h>

char *xstrdup(const char *str)
{
	size_t len = strlen(str);
	char *dup = new char [len + 1];
	strcpy(dup, str);

	return dup;
}

//------------------------------

static void xerror_handler(
	int severity,
	po_message_t message, const char *filename, size_t lineno,
	size_t column, int multiline_p, const char *message_text)
{
	printf("filename = %s, lineno = %lu, column = %lu\n", filename, lineno, column);
//	assert(0);
}

static void xerror2_handler(
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

// overloaded function, without 'xerror_handlers' argument
po_file_t po_file_write(po_file_t file, const char *filename)
{
	struct po_xerror_handler xerror_handlers;
	xerror_handlers.xerror = xerror_handler;
	xerror_handlers.xerror2 = xerror2_handler;

	po_file_t res = po_file_write(file, filename, &xerror_handlers);
	if (res == NULL)
	{
		printf("Cannot write .po/.pot file: %s\n", filename);
		assert(0);
	}

	return res;
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
		assert(write(pipe_fd[1], buffer, length) == length);

		exit(0); // terminate child process
	}

	// In parent process now

	close(pipe_fd[1]);
	po_file_t file = po_file_read("-"); // read from stdin

	// Restore 'stdin'
	close(0);
	assert(dup2(stdin_fd, 0) >= 0);
	close(stdin_fd);

	// Kill zombie child process, check exit status
	int status = -1;
	assert(waitpid(pid, &status, 0) == pid);
	assert(status == 0);

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

	if (!isalpha(header[1]))
	{
		// Header corrupt. See KDE SVN revision 1228594 for an example.
		throw TranslationContent::ExceptionNotPo();
	}

	// find the "POT-Creation-Date:" field
	const char *pot_creation_pattern = "\nPOT-Creation-Date: "; // text that goes before the POT creation date
	char *pot_creation_str = strstr(header, pot_creation_pattern);
	if (pot_creation_str == NULL)
	{
		// There must be a "POT-Creation-Date:" in the header.

		printf("header:\n[%s]\n", header);
		assert(0);
	}

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

std::string calculate_tp_hash(const char *filename)
{
	TranslationContent *content = new TranslationContent(filename);
	char res_str[GIT_OID_HEXSZ + 1];
	git_oid_fmt(res_str, content->calculateTpHash());
	res_str[GIT_OID_HEXSZ] = '\0';

	delete content;

	return std::string(res_str);
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

	// free memory
	po_message_iterator_free(iterator);
	po_file_free(file);

	return length;
}

//-------- Coupling IDs of equal messages in different .po/.pot files -------

std::vector<std::pair<std::string, int> > dump_po_file_ids(TranslationContent *content, int first_id)
{
	std::vector<std::pair<std::string, int> > res;

	po_file_t file = content->poFileRead(); // all checks and error reporting are done in po_file_read

	assert(file);
//	if (!file)
//		return res;


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

	// free memory
	po_message_iterator_free(iterator);
	po_file_free(file);

	return res;
}

std::vector<std::vector<int> > list_equal_messages_ids(std::vector<std::pair<TranslationContent *, int> > files)
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

std::vector<std::pair<int, int> > list_equal_messages_ids_2(TranslationContent *file_a, int first_id_a, TranslationContent *file_b, int first_id_b)
{
	std::vector<std::pair<TranslationContent *, int> > files;
	files.push_back(std::pair<TranslationContent *, int>(file_a, first_id_a));
	files.push_back(std::pair<TranslationContent *, int>(file_b, first_id_b));

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

int dump_equal_messages_to_mmapdb(TranslationContent *file_a, int first_id_a, TranslationContent *file_b, int first_id_b, MappedFileIdMapDb *mmap_db)
{
	std::vector<std::pair<int, int> > list = list_equal_messages_ids_2(
		file_a, first_id_a, file_b, first_id_b);
	for (std::vector<std::pair<int, int> >::iterator iter = list.begin(); iter != list.end(); iter ++)
		mmap_db->addRow(iter->first, iter->second);

	return (int)list.size();
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

