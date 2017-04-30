#include <gtest/gtest.h>

#include "common.h"

#include <gtpo/iddiff.h>
#include <gtpo/translationcontent.h>
#include <gtpo/filecontentfs.h>

#include <sstream>

TEST(MatchPoMessages, Match) {
    std::string inputDir(INPUT_DATA_DIR "/matchpomessages/");

    TranslationContent *file_a = new TranslationContent(new FileContentFs(inputDir + "data/dumpids-a.po"));
    TranslationContent *file_b = new TranslationContent(new FileContentFs(inputDir + "data/dumpids-b.po"));
    std::vector<std::pair<int, int> > list = list_equal_messages_ids_2(
        file_a, 1000,
        file_b, 101000);

    std::ostringstream output;
    for (size_t i = 0; i < list.size(); i ++) {
        output << list[i].first << " " << list[i].second << std::endl;
    }

    std::string reference = read_file_contents((inputDir + "ref/list_equal_messages_ids.txt").c_str());

    EXPECT_EQ(reference, output.str());
}
