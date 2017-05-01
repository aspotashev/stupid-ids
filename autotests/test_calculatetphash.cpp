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
TEST(TranslationContent, TpHash) {
    EXPECT_EQ("50af8de1177dce3f94deb2d866d49f0264aae2fd", getTpHash("check-tphash.pot"));
    EXPECT_EQ("50af8de1177dce3f94deb2d866d49f0264aae2fd", getTpHash("check-tphash.po"));

    EXPECT_EQ("fd48fbf951a961c89e2760110280e9777cf1593e", getTpHash("base.po"));
    EXPECT_EQ("fd48fbf951a961c89e2760110280e9777cf1593e", getTpHash("change-trcomments.po"));

    EXPECT_EQ("ffe72f31b23497e764a8c4b4d42c30d5a32dba7c", getTpHash("change-msgid.po"));
    EXPECT_EQ("2b3a5aabb62a761744451cca6454fbdb19fe0e79", getTpHash("change-msgctxt.po"));
    EXPECT_EQ("8f650eac3dc1c739dd5a0e170a303249bf427228", getTpHash("change-msgidplural.po"));
    EXPECT_EQ("98497a3c9d81b04f2bea35c6e4cfc6f99cd188f9", getTpHash("change-autocomments.po"));
    EXPECT_EQ("22ee08d28354af5eb031bee161158e3157f607e4", getTpHash("change-sourcepos.po"));
    EXPECT_EQ("00b62471842fc3c05333b8a4e3b2c729f58cfb9f", getTpHash("change-sourcefile.po"));
    EXPECT_EQ("99a0ced3f721df2a5aabbb7dd08574499a21c329", getTpHash("change-potdate.po"));
}
