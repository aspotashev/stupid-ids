
// http://www.gnu.org/software/gettext/manual/gettext.html#libgettextpo
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <gettext-po.h>

void xerror_handler(
	int severity,
	po_message_t message, const char *filename, size_t lineno,
	size_t column, int multiline_p, const char *message_text)
{
	printf("filename = %s, lineno = %lu, column = %lu\n", filename, lineno, column);
	assert(0);
}

void xerror2_handler(
	int severity,
	po_message_t message1, const char *filename1, size_t lineno1,
	size_t column1, int multiline_p1, const char *message_text1,
	po_message_t message2, const char *filename2, size_t lineno2,
	size_t column2, int multiline_p2, const char *message_text2)
{
	assert(0);
}

std::string wrap_string(const char *str)
{
	std::string res;

	size_t len = strlen(str);
	char hex[3];
	for (size_t i = 0; i < len; i ++)
	{
		sprintf(hex, "%02x", str[i]);
		res += hex;
	}

	return res;
}

std::string wrap_po_message(po_message_t message)
{
	if (*po_message_msgid(message) == '\0') // header
	{
		return "";
	}

	if (po_message_is_obsolete(message) || po_message_is_fuzzy(message))
	{
		printf("obsolete of fuzzy message found\n");
		assert(0);
	}

	std::string res = wrap_string(po_message_msgid(message)); // msgid
	if (po_message_msgid_plural(message))
	{
		res += "_";
		res += wrap_string(po_message_msgid_plural(message));
	}

	res += " ";
	if (po_message_msgstr_plural(message, 0)) // message has plural forms
		res += po_message_msgstr_plural(message, 0);
	else // single msgstr
		res += po_message_msgstr(message);

	return res;
}

void dump_po_file_ids(const char *filename)
{
	struct po_xerror_handler xerror_handlers;
	xerror_handlers.xerror = xerror_handler;
	xerror_handlers.xerror2 = xerror2_handler;

	po_file_t file = po_file_read(filename, &xerror_handlers);
	if (file == NULL)
	{
		printf("Cannot read .po file: %s\n", filename);
		assert(0);
	}

	const char * const *domains = po_file_domains(file);
	assert(strcmp(domains[0], "messages") == 0);
	assert(domains[1] == NULL);

	// main cycle
	po_message_iterator_t iterator = po_message_iterator(file, "messages");
	po_message_t message; // in fact, this is a pointer
	for (int index = 0; message = po_next_message(iterator); index ++)
	{
		std::string msg_dump = wrap_po_message(message);
		if (msg_dump.length() > 0)
			std::cout << msg_dump << std::endl;
	}

	po_file_free(file); // free memory
}

int main()
{
	dump_po_file_ids("temp/dolphin.pot");

	return 0;
}

