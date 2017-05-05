#include <gtest/gtest.h>

#include "common.h"

#include <gtpo/iddiff.h>
#include <gtpo/iddiffmessage.h>
#include <gtpo/translationcontent.h>
#include <gtpo/filecontentfs.h>

rapidjson::Document parseJson(std::string s)
{
    rapidjson::Document doc;
    doc.Parse(s.c_str());

    return doc;
}

bool jsonEqual(std::string json1, std::string json2)
{
    return parseJson(json1) == parseJson(json2);
}

// Testing here:
// Iddiffer::loadIddiff()
// Iddiffer::generateIddiffText()
TEST(IddiffTest, LoadSave)
{
    std::string inputfile_str;
    inputfile_str += INPUT_DATA_DIR;
    inputfile_str += "/iddiff/";
    inputfile_str += "1.iddiff";

    std::string input = read_file_contents(inputfile_str.c_str());

    Iddiff* diff = new Iddiff();
    diff->loadIddiff(inputfile_str.c_str());
    std::string output = diff->generateIddiffText();

    EXPECT_TRUE(jsonEqual(input, output));
}

TEST(IddiffTest, Build1)
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

    EXPECT_TRUE(jsonEqual(input, output));
}

TEST(IddiffTest, MergeHeaders)
{
    std::string inputDir(INPUT_DATA_DIR "/iddiff/");

    Iddiff *diff_a = new Iddiff();
    assert(diff_a->loadIddiff((inputDir + "merge_a.iddiff").c_str()));
    Iddiff *diff_b = new Iddiff();
    assert(diff_b->loadIddiff((inputDir + "merge_b.iddiff").c_str()));

    diff_a->mergeHeaders(diff_b);
    std::string output = diff_a->generateIddiffText();

    std::string reference = read_file_contents((inputDir + "merged.iddiff").c_str());

    EXPECT_TRUE(jsonEqual(reference, output));
}

TEST(IddiffTest, DiffAgaistEmpty)
{
    std::string inputDir(INPUT_DATA_DIR "/iddiff/against-empty/");

    TranslationContent* content = new TranslationContent(new FileContentFs(inputDir + "input.po"));

    {
        Iddiff* diff = new Iddiff();
        diff->diffAgainstEmpty(content);
        EXPECT_TRUE(jsonEqual(
            diff->generateIddiffText(),
            read_file_contents(inputDir + "diff.iddiff")));
    }

    {
        Iddiff* diff_trcomments = new Iddiff();
        diff_trcomments->diffTrCommentsAgainstEmpty(content);
        EXPECT_TRUE(jsonEqual(
            diff_trcomments->generateIddiffText(),
            read_file_contents(inputDir + "diff-trcomments.iddiff")));
    }
}
