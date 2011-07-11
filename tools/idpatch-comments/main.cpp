
#include <assert.h>
#include <iostream>

#include <gettextpo-helper/translation-collector.h>
#include <gettextpo-helper/iddiff.h>


// TODO: merge into "tools/idpatch" (as optional mode)
int main(int argc, char *argv[])
{
	assert(argc == 2); // 1 argument (path to .iddiff)

	Iddiffer *differ = new Iddiffer();
	differ->loadIddiff(argv[1]);
	differ->minimizeIds();

	std::cout << differ->generateIddiffText();

	// Apply patch
	StupIdTranslationCollector collector;
	collector.insertPoDir("/home/sasha/kde-ru/clean-svn/trunk");
	collector.insertPoDir("/home/sasha/kde-ru/clean-svn/stable");

	differ->applyIddiffComments(&collector);

	return 0;
}

