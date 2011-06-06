
#include <assert.h>
#include <iostream>

//#include <gettextpo-helper/translationcontent.h>
#include <gettextpo-helper/translation-collector.h>
#include <gettextpo-helper/iddiff.h>


int main(int argc, char *argv[])
{
	assert(argc == 2); // 1 argument (path to .iddiff)

	Iddiffer *differ = new Iddiffer();
	differ->loadIddiff(argv[1]);
	differ->minimizeIds();

	std::cout << differ->generateIddiffText();

	// Apply patch
	StupIdTranslationCollector collector;
	collector.insertPoDir("/home/sasha/kde-ru/kde-ru-trunk.git");
	collector.insertPoDir("/home/sasha/kde-ru/kde-l10n-ru-stable");

	differ->applyIddiff(&collector);

	return 0;
}

