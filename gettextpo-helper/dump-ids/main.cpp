
// http://www.gnu.org/software/gettext/manual/gettext.html#libgettextpo
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <vector>
#include <map>

#include <gettextpo-helper/gettextpo-helper.h>
#include <gettextpo-helper/translationcontent.h>

int main(int argc, char *argv[])
{
	assert(argc == 5); // 4 arguments

	TranslationContent *file_a = new TranslationContent(argv[1]);
	TranslationContent *file_b = new TranslationContent(argv[3]);
	std::vector<std::pair<int, int> > list = list_equal_messages_ids_2(
		file_a, atoi(argv[2]),
		file_b, atoi(argv[4]));
	for (size_t i = 0; i < list.size(); i ++)
		std::cout << list[i].first << " " << list[i].second << std::endl;

	delete file_a;
	delete file_b;

	return 0;
}

