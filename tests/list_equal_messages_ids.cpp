
#include <fstream>

#include <gettextpo-helper/gettextpo-helper.h>
#include <gettextpo-helper/translationcontent.h>

int main(int argc, char *argv[])
{
	TranslationContent *file_a = new TranslationContent(FILES_DATA_DIR "dumpids-a.po");
	TranslationContent *file_b = new TranslationContent(FILES_DATA_DIR "dumpids-b.po");
	std::vector<std::pair<int, int> > list = list_equal_messages_ids_2(
		file_a, 1000,
		file_b, 101000);

	std::ofstream out("result.txt");
	for (size_t i = 0; i < list.size(); i ++)
		out << list[i].first << " " << list[i].second << std::endl;

	return 0;
}

