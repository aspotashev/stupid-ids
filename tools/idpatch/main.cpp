
//#include <assert.h>
#include <iostream>

//#include <gettextpo-helper/translationcontent.h>
#include <gettextpo-helper/translation-collector.h>
#include <gettextpo-helper/iddiff.h>


int main()
{
	Iddiffer *differ = new Iddiffer();
	differ->loadIddiff("../iddiff/amarok.iddiff");
	differ->minimizeIds();

	std::cout << differ->generateIddiffText();

	// Apply patch
	StupIdTranslationCollector collector;
	collector.insertPoDir("/home/sasha/kde-ru/kde-ru-trunk.git");
	collector.insertPoDir("/home/sasha/kde-ru/kde-l10n-ru-stable");

	differ->applyIddiff(&collector);

	return 0;
}

