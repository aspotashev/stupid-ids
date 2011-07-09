#include <iostream>
#include <gettextpo-helper/translationcontent.h>
#include <gettextpo-helper/detectorbase.h>

// TODO: use in "sha1_buffer" test
void assertEqualOids(GitOid a, GitOid b, const char *testname)
{
	if (a != b)
	{
		std::cout << "OIDs differ: [" << testname << "]" << std::endl;
		std::cout << "a = " << a.toString() << std::endl;
		std::cout << "b = " << b.toString() << std::endl;
		abort(); // fail
	}
}

void runTest(const char *sha1, const char *inputfile)
{
	std::string inputfile_str;
	inputfile_str += INPUT_DATA_DIR;
	inputfile_str += inputfile;

	TranslationContent *content = new TranslationContent(inputfile_str.c_str());
	assertEqualOids(GitOid(content->calculateTpHash()), GitOid(sha1), inputfile);

	delete content;
}

int main()
{
	runTest("3daba7a6acba93eb449bc6fe5a28acfda7d7bad0", "check-tphash.pot");
	runTest("3daba7a6acba93eb449bc6fe5a28acfda7d7bad0", "check-tphash.po");


	runTest("181f2933566bba8b19192b7384bd5a0fb835ec47", "base.po");
	runTest("181f2933566bba8b19192b7384bd5a0fb835ec47", "change-trcomments.po");

	runTest("8ca4a81c09b2f811f0f7722f09e2fb0377983601", "change-msgid.po");
	runTest("73496ffb20d1352eed2fb5ec2097f0b553f4ab03", "change-msgctxt.po");
	runTest("8aa616d15a5b6e9ed451470038607bce31228efb", "change-msgidplural.po");
	runTest("bcf8f67a9dd15705d3884aea8ad9a5c9547d0fb5", "change-autocomments.po");
	runTest("72919da53f4a6caabb1c3a34885b53d21a50a011", "change-sourcepos.po");
	runTest("84d0afc1b22b1515bd759c1f5449baf68c9beed7", "change-sourcefile.po");
	runTest("6a0f6b9f8b7ce107944554512465e6882ccb2f43", "change-potdate.po");

	return 0; // ok
}

