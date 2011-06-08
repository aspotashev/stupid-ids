
#include <assert.h>
#include <iostream>

#include <gettextpo-helper/translationcontent.h>
#include <gettextpo-helper/iddiff.h>
#include <gettextpo-helper/gitloader.h>


int main(int argc, char *argv[])
{
	assert(argc == 2); // 1 argument

	TranslationContent *new_content = new TranslationContent(argv[1]);

	// For loading blobs by their OIDs
	GitLoader *git_loader = new GitLoader();
	git_loader->addRepository("/home/sasha/kde-ru/kde-ru-trunk.git/.git");
	git_loader->addRepository("/home/sasha/kde-ru/kde-l10n-ru-stable/.git");

	TranslationContent *old_content = git_loader->findOldestByTphash(new_content->calculateTpHash());
	assert(old_content);

	std::cout << Iddiffer::generateIddiffText(old_content, new_content);

	return 0;
}

