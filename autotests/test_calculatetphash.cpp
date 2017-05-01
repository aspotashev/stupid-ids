#include <gtest/gtest.h>

#include <gtpo/translationcontent.h>
#include <gtpo/filecontentfs.h>
#include <gtpo/gitoid.h>

class TranslationContentTest
    : public ::testing::TestWithParam<std::tuple<const char*, const char*>> {
protected:
    const char* filename() const {
        return std::get<0>(GetParam());
    }

    const char* expectedTphash() const {
        return std::get<1>(GetParam());
    }
};

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
TEST_P(TranslationContentTest, TpHash) {
    EXPECT_EQ(expectedTphash(), getTpHash(filename()));
}

INSTANTIATE_TEST_CASE_P(
    TestWithParameters, TranslationContentTest, testing::Values(
        std::make_tuple("check-tphash.pot", "50af8de1177dce3f94deb2d866d49f0264aae2fd"),
        std::make_tuple("check-tphash.po", "50af8de1177dce3f94deb2d866d49f0264aae2fd"),

        std::make_tuple("base.po", "fd48fbf951a961c89e2760110280e9777cf1593e"),
        std::make_tuple("change-trcomments.po", "fd48fbf951a961c89e2760110280e9777cf1593e"),

        std::make_tuple("change-msgid.po", "ffe72f31b23497e764a8c4b4d42c30d5a32dba7c"),
        std::make_tuple("change-msgctxt.po", "2b3a5aabb62a761744451cca6454fbdb19fe0e79"),
        std::make_tuple("change-msgidplural.po", "8f650eac3dc1c739dd5a0e170a303249bf427228"),
        std::make_tuple("change-autocomments.po", "98497a3c9d81b04f2bea35c6e4cfc6f99cd188f9"),
        std::make_tuple("change-sourcepos.po", "22ee08d28354af5eb031bee161158e3157f607e4"),
        std::make_tuple("change-sourcefile.po", "00b62471842fc3c05333b8a4e3b2c729f58cfb9f"),
        std::make_tuple("change-potdate.po", "99a0ced3f721df2a5aabbb7dd08574499a21c329")
    ));
