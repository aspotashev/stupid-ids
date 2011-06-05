
#include <assert.h>
#include <iostream>

#include <gettextpo-helper/translationcontent.h>
#include <gettextpo-helper/iddiff.h>


int main(int argc, char *argv[])
{
	assert(argc == 3); // 2 arguments

	std::cout << Iddiffer::generateIddiffText(
		new TranslationContent(argv[1]),
		new TranslationContent(argv[2]));

	return 0;
}

