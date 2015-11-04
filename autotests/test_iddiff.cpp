#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <gtpo/iddiff.h>
#include <gtpo/iddiffmessage.h>

#include <string>
#include <sstream>
#include <fstream>
#include <stdexcept>

BOOST_AUTO_TEST_SUITE(IddiffTest)

std::string read_file_contents(std::string filename)
{
    std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);
    if (!in) {
        std::stringstream ss;
        ss << "Failed to open file " << filename;
        throw std::runtime_error(ss.str());
    }

    std::ostringstream ss;
    ss << in.rdbuf();
    in.close();
    return ss.str();
}

// Testing here:
// Iddiffer::loadIddiff()
// Iddiffer::generateIddiffText()
BOOST_AUTO_TEST_CASE(LoadSave)
{
    std::string inputfile_str;
    inputfile_str += INPUT_DATA_DIR;
    inputfile_str += "/iddiff/";
    inputfile_str += "1.iddiff";

    std::string input = read_file_contents(inputfile_str.c_str());

    Iddiff* diff = new Iddiff();
    diff->loadIddiff(inputfile_str.c_str());
    std::string output = diff->generateIddiffText();

    rapidjson::Document docInput;
    docInput.Parse(input.c_str());
    rapidjson::Document docOutput;
    docOutput.Parse(output.c_str());

    BOOST_CHECK(docInput == docOutput);
}

BOOST_AUTO_TEST_CASE(Build1)
{
    std::string inputfile_str;
    inputfile_str += INPUT_DATA_DIR;
    inputfile_str += "/iddiff/";
    inputfile_str += "1.iddiff";

    std::string input = read_file_contents(inputfile_str.c_str());

    Iddiff* diff = new Iddiff();

    {
        IddiffMessage *msg1 = new IddiffMessage();
        msg1->addMsgstr(OptString("%1 объект"));
        msg1->addMsgstr(OptString("%1 объекта"));
        msg1->addMsgstr(OptString("%1 объектов"));
        msg1->addMsgstr(OptString("%1 объект"));
        diff->insertAdded(39147300, msg1);
    }

    {
        IddiffMessage *msg2 = new IddiffMessage();
        msg2->addMsgstr(OptString("Очистить список закрытых вкладок"));
        diff->insertAdded(39147465, msg2);
    }

    {
        IddiffMessage *msg3 = new IddiffMessage();
        msg3->setFuzzy(true);
        msg3->addMsgstr(OptString("Недавно закрытые вкладки"));
        diff->insertRemoved(39147465, msg3);
    }

    {
        IddiffMessage *msg4 = new IddiffMessage();
        msg4->addMsgstr(OptString("bla bla"));
        diff->insertReview(39147465, msg4);
    }

    std::string output = diff->generateIddiffText();

    rapidjson::Document docInput;
    docInput.Parse(input.c_str());
    rapidjson::Document docOutput;
    docOutput.Parse(output.c_str());

    BOOST_CHECK(docInput == docOutput);
}

BOOST_AUTO_TEST_CASE(MergeHeaders)
{
    std::string inputDir(INPUT_DATA_DIR "/iddiff/");

    Iddiff *diff_a = new Iddiff();
    assert(diff_a->loadIddiff((inputDir + "merge_a.iddiff").c_str()));
    Iddiff *diff_b = new Iddiff();
    assert(diff_b->loadIddiff((inputDir + "merge_b.iddiff").c_str()));

    diff_a->mergeHeaders(diff_b);
    std::string output = diff_a->generateIddiffText();

    std::string reference = read_file_contents((inputDir + "merged.iddiff").c_str());

    rapidjson::Document docRef;
    docRef.Parse(reference.c_str());
    rapidjson::Document docOutput;
    docOutput.Parse(output.c_str());

    BOOST_CHECK(docRef == docOutput);
}

BOOST_AUTO_TEST_SUITE_END()
