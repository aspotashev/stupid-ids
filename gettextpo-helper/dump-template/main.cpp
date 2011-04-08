
// http://www.gnu.org/software/gettext/manual/gettext.html#libgettextpo
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <gettext-po.h>
#include <cryptopp/sha.h> // CryptoPP::SHA1
#include <cryptopp/filters.h> // CryptoPP::StringSource
#include <cryptopp/hex.h> // CryptoPP::HexEncoder

#include "../include/gettextpo-helper.h"

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

std::string wrap_template_message(po_message_t message)
{
	assert(*po_message_msgid(message) != '\0'); // header was processed separately

	if (po_message_is_obsolete(message)) // obsolete messages should not affect the dump
		return std::string();


	std::string res;

	res += "T"; // "T". May be NULL.
	res += wrap_string_hex(po_message_msgctxt(message));

	res += "M"; // "M". Cannot be NULL or empty.
	res += wrap_string_hex(po_message_msgid(message));

	res += "P"; // "P". May be NULL.
	res += wrap_string_hex(po_message_msgid_plural(message));

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
		std::string msg_dump = wrap_template_message(message);
		if (msg_dump.length() > 0) // non-obsolete
		{
			res += "/";
			res += msg_dump;
		}
	}

	po_file_free(file); // free memory
	return res;
}

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

int main(int argc, char *argv[])
{
	const char *filename = NULL;
	if (argc == 1)
		filename = "-";
	else if (argc == 2)
		filename = argv[1];
	else
		assert(0); // too many arguments


	std::string dump = dump_po_file_template(filename);
	std::cout << sha1_string(dump);

	return 0;
}

