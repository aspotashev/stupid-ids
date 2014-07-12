#include <gtpo/translationcontent.h>
#include <gtpo/iddiff.h>

#include <iostream>
#include <cassert>

int main(int argc, char *argv[])
{
	Iddiffer *diff = new Iddiffer();
	Iddiffer *diff_trcomments = new Iddiffer();
	if (argc == 2) // 1 argument
	{
		TranslationContent *content = new TranslationContent(argv[1]);
		diff->diffAgainstEmpty(content);
		diff_trcomments->diffTrCommentsAgainstEmpty(content);
	}
	else if (argc == 3)
	{
		TranslationContent *content_a = new TranslationContent(argv[1]);
		TranslationContent *content_b = new TranslationContent(argv[2]);
		diff->diffFiles(content_a, content_b);
		diff_trcomments->diffTrCommentsFiles(content_a, content_b);
	}
	else
	{
		std::cerr << "Invalid number of arguments (" << argc - 1 << ")" << std::endl;
		abort();
	}

	std::cout << diff->generateIddiffText();

	// TODO: add an option to choose path to .iddiff-trcomm or disable dumping of comments
	diff_trcomments->writeToFile("/tmp/11235.iddiff-tr-comments");

	return 0;
}

