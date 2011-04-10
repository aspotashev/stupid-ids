
// http://www.gnu.org/software/gettext/manual/gettext.html#libgettextpo
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <vector>
#include <map>

#include "../include/gettextpo-helper.h"

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
		std::string msg_dump = wrap_template_message(message);
		if (msg_dump.length() > 0)
			res.push_back(make_pair(msg_dump, first_id + index));
	}

	po_file_free(file); // free memory
	return res;
}

typedef std::map<std::string, std::vector<int> > msg_ids_map;

int main(int argc, char *argv[])
{
	std::vector<std::pair<const char *, int> > files;

	assert(argc == 5); // 4 arguments
	files.push_back(std::pair<const char *, int>(argv[1], atoi(argv[2])));
	files.push_back(std::pair<const char *, int>(argv[3], atoi(argv[4])));

	msg_ids_map msg_ids;
	for (size_t d = 0; d < files.size(); d ++)
	{
		std::vector<std::pair<std::string, int> > dump = dump_po_file_ids(files[d].first, files[d].second);
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

