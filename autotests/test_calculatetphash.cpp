#include <gtest/gtest.h>

#include <gtpo/translationcontent.h>
#include <gtpo/filecontentfs.h>
#include <gtpo/gitoid.h>

static std::string getTpHash(const char* inputfile)
{
    std::string inputfile_str;
    inputfile_str += INPUT_DATA_DIR;
    inputfile_str += "/tp_hash/";
    inputfile_str += inputfile;

    TranslationContent *content = new TranslationContent(new FileContentFs(inputfile_str));
    std::string res = content->calculateTpHash().toString();

    delete content;

    return res;
}

// Testing here:
// TranslationContent::calculateTpHash()
TEST(TpHash, Main) {
    EXPECT_EQ("3daba7a6acba93eb449bc6fe5a28acfda7d7bad0", getTpHash("check-tphash.pot"));
    EXPECT_EQ("3daba7a6acba93eb449bc6fe5a28acfda7d7bad0", getTpHash("check-tphash.po"));


    EXPECT_EQ("181f2933566bba8b19192b7384bd5a0fb835ec47", getTpHash("base.po"));
    EXPECT_EQ("181f2933566bba8b19192b7384bd5a0fb835ec47", getTpHash("change-trcomments.po"));

    EXPECT_EQ("8ca4a81c09b2f811f0f7722f09e2fb0377983601", getTpHash("change-msgid.po"));
    EXPECT_EQ("73496ffb20d1352eed2fb5ec2097f0b553f4ab03", getTpHash("change-msgctxt.po"));
    EXPECT_EQ("8aa616d15a5b6e9ed451470038607bce31228efb", getTpHash("change-msgidplural.po"));
    EXPECT_EQ("bcf8f67a9dd15705d3884aea8ad9a5c9547d0fb5", getTpHash("change-autocomments.po"));
    EXPECT_EQ("72919da53f4a6caabb1c3a34885b53d21a50a011", getTpHash("change-sourcepos.po"));
    EXPECT_EQ("84d0afc1b22b1515bd759c1f5449baf68c9beed7", getTpHash("change-sourcefile.po"));
    EXPECT_EQ("6a0f6b9f8b7ce107944554512465e6882ccb2f43", getTpHash("change-potdate.po"));
}
