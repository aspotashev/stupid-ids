#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include "common.h"

#include <gtpo/iddiff.h>
#include <gtpo/translationcontent.h>

#include <sstream>

BOOST_AUTO_TEST_SUITE(MatchPoMessages)

BOOST_AUTO_TEST_CASE(Match)
{
    std::string inputDir(INPUT_DATA_DIR "/matchpomessages/");

    TranslationContent *file_a = new TranslationContent(inputDir + "data/dumpids-a.po");
    TranslationContent *file_b = new TranslationContent(inputDir + "data/dumpids-b.po");
    std::vector<std::pair<int, int> > list = list_equal_messages_ids_2(
        file_a, 1000,
        file_b, 101000);

    std::ostringstream output;
    for (size_t i = 0; i < list.size(); i ++) {
        output << list[i].first << " " << list[i].second << std::endl;
    }

    std::string reference = read_file_contents((inputDir + "ref/list_equal_messages_ids.txt").c_str());

    BOOST_CHECK_EQUAL(reference, output.str());
}

BOOST_AUTO_TEST_SUITE_END()
