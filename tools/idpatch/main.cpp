
#include <assert.h>
#include <stdio.h>
#include <iostream>

#include <gettextpo-helper/translation-collector.h>
#include <gettextpo-helper/iddiff.h>


int main(int argc, char *argv[])
{
	assert(argc == 3); // 2 arguments (mode; path to .iddiff)

	bool comments = false;
	if (!strcmp(argv[1], "messages"))
		comments = false;
	else if (!strcmp(argv[1], "comments"))
		comments = true;
	else
	{
		fprintf(stderr, "Unknown command: %s\n", argv[1]);
		return 1;
	}

	Iddiffer *differ = new Iddiffer();
	differ->loadIddiff(argv[1]);
	differ->minimizeIds();

	std::cout << differ->generateIddiffText();

	// Apply patch
	StupIdTranslationCollector collector;
	collector.insertPoDirOrTemplate("/home/sasha/kde-ru/xx-numbering/templates", "/home/sasha/kde-ru/clean-svn/trunk");
	collector.insertPoDirOrTemplate("/home/sasha/kde-ru/xx-numbering/stable-templates", "/home/sasha/kde-ru/clean-svn/stable");

	if (comments)
		differ->applyIddiffComments(&collector);
	else
		differ->applyIddiff(&collector);

	return 0;
}

