#include <gtpo/gettextpo-helper.h>
#include <gtpo/translationcontent.h>

#include <cassert>
#include <iostream>
#include <vector>
#include <utility>

int main(int argc, char *argv[])
{
    assert(argc == 5); // 4 arguments

    TranslationContent *file_a = new TranslationContent(argv[1]);
    TranslationContent *file_b = new TranslationContent(argv[3]);
    std::vector<std::pair<int, int> > list = list_equal_messages_ids_2(
        file_a, atoi(argv[2]),
        file_b, atoi(argv[4]));

    for (size_t i = 0; i < list.size(); i ++) {
        std::cout << list[i].first << " " << list[i].second << std::endl;
    }

    delete file_a;
    delete file_b;

    return 0;
}
