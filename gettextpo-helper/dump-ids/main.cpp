
// http://www.gnu.org/software/gettext/manual/gettext.html#libgettextpo
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <vector>
#include <map>

#include <gettextpo-helper.h>

int main(int argc, char *argv[])
{
	assert(argc == 5); // 4 arguments

	std::vector<std::pair<int, int> > list = list_equal_messages_ids_2(argv[1], atoi(argv[2]), argv[3], atoi(argv[4]));
	for (size_t i = 0; i < list.size(); i ++)
		std::cout << list[i].first << " " << list[i].second << std::endl;

	return 0;
}

