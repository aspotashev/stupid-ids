
// http://www.gnu.org/software/gettext/manual/gettext.html#libgettextpo
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <vector>
#include <map>
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

std::pair<std::string, std::string> wrap_po_message(po_message_t message)
{
	if (*po_message_msgid(message) == '\0') // header
	{
		return make_pair(std::string(), std::string());
	}

	if (po_message_is_obsolete(message) || po_message_is_fuzzy(message))
	{
		printf("obsolete of fuzzy message found\n");
		assert(0);
	}

	std::string res1 = wrap_string(po_message_msgid(message)); // msgid
	if (po_message_msgid_plural(message))
	{
		res1 += "_";
		res1 += wrap_string(po_message_msgid_plural(message));
	}

	// FIXME: msgctxt should also be dumped!

	std::string res2;
	if (po_message_msgstr_plural(message, 0)) // message has plural forms
		res2 = po_message_msgstr_plural(message, 0);
	else // single msgstr
		res2 = po_message_msgstr(message);

	return make_pair(res1, res2);
}

std::vector<std::pair<std::string, std::string> > dump_po_file_ids(const char *filename)
{
	std::vector<std::pair<std::string, std::string> > res;

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
		std::pair<std::string, std::string> msg_dump = wrap_po_message(message);
		if (msg_dump.first.length() > 0)
			res.push_back(msg_dump);
	}

	po_file_free(file); // free memory
	return res;
}

typedef std::map<std::string, std::vector<std::string> > msg_ids_map;

int main()
{
	std::vector<const char *> files;
	files.push_back("temp/dolphin.pot");
	files.push_back("temp/stable-dolphin.pot");

	msg_ids_map msg_ids;
	for (size_t d = 0; d < files.size(); d ++)
	{
		std::vector<std::pair<std::string, std::string> > dump = dump_po_file_ids(files[d]);
		for (size_t i = 0; i < dump.size(); i ++)
			msg_ids[dump[i].first].push_back(dump[i].second);
	}

	for (msg_ids_map::iterator iter = msg_ids.begin(); iter != msg_ids.end(); iter ++)
	{
		size_t n_ids = iter->second.size();
		if (n_ids == 0)
		{
			assert(0);
		}
		else if (n_ids >= 2)
		{
			std::cout << iter->second[0];
			for (size_t i = 1; i < n_ids; i ++)
				std::cout << " " << iter->second[i];

			std::cout << std::endl;
		}
	}

	return 0;
}

