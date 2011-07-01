
#include <assert.h>
#include <iostream>

#include <gettextpo-helper/translationcontent.h>
#include <gettextpo-helper/iddiff.h>


int main(int argc, char *argv[])
{
	Iddiffer *diff = new Iddiffer();
	if (argc == 2) // 1 argument
		diff->diffAgainstEmpty(new TranslationContent(argv[1]));
	else if (argc == 3)
		diff->diffFiles(
			new TranslationContent(argv[1]),
			new TranslationContent(argv[2]));
	else
	{
		std::cerr << "Invalid number of arguments (" << argc - 1 << ")" << std::endl;
		abort();
	}

	std::cout << diff->generateIddiffText();
	return 0;
}

