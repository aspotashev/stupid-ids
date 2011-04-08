
// http://www.gnu.org/software/gettext/manual/gettext.html#libgettextpo
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <gettext-po.h>

#include "../include/gettextpo-helper.h"

std::pair<std::string, std::string> wrap_po_message(po_message_t message)
{
	if (*po_message_msgid(message) == '\0') // header
	{
		return make_pair(std::string(), std::string());
	}

	if (po_message_is_obsolete(message) || po_message_is_fuzzy(message))
	{
		printf("obsolete or fuzzy message found\n");
		assert(0);
	}

	std::string res1 = wrap_string_hex(po_message_msgid(message)); // msgid
	if (po_message_msgid_plural(message))
	{
		res1 += "P"; // plural
		res1 += wrap_string_hex(po_message_msgid_plural(message));
	}

	if (po_message_msgctxt(message)) // msgctxt
	{
		res1 += "T"; // context
		res1 += wrap_string_hex(po_message_msgctxt(message));
	}

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

	po_file_t file = po_file_read(filename); // all checks and error reporting are done in po_file_read

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
			assert(n_ids == 2);

			std::cout << iter->second[0];
			for (size_t i = 1; i < n_ids; i ++)
				std::cout << " " << iter->second[i];

			std::cout << std::endl;
		}
	}

	return 0;
}

