
// http://www.gnu.org/software/gettext/manual/gettext.html#libgettextpo
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <vector>
#include <map>

#include "../include/gettextpo-helper.h"

int main(int argc, char *argv[])
{
	std::vector<std::pair<const char *, int> > files;

	assert(argc == 5); // 4 arguments
	files.push_back(std::pair<const char *, int>(argv[1], atoi(argv[2])));
	files.push_back(std::pair<const char *, int>(argv[3], atoi(argv[4])));

	std::vector<std::vector<int> > list = list_equal_messages_ids(files);

	for (size_t i = 0; i < list.size(); i ++)
	{
		std::vector<int> ids = list[i];

		std::cout << ids[0];
		for (size_t j = 1; j < ids.size(); j ++)
			std::cout << " " << ids[j];
		std::cout << std::endl;
	}

	return 0;
}

