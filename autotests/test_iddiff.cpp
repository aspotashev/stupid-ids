#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <gtpo/iddiff.h>

#include <string>
#include <sstream>
#include <fstream>
#include <stdexcept>

BOOST_AUTO_TEST_SUITE(IddiffTest)

std::string read_file_contents(const char *filename)
{
    std::ifstream in(filename, std::ios::in | std::ios::binary);
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

BOOST_AUTO_TEST_SUITE_END()
