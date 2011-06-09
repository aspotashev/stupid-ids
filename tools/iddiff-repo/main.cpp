
#include <assert.h>
#include <stdio.h>
#include <iostream>

#include <gettextpo-helper/translationcontent.h>
#include <gettextpo-helper/iddiff.h>
#include <gettextpo-helper/gitloader.h>


int main(int argc, char *argv[])
{
	// For loading blobs by their OIDs
	GitLoader *git_loader = new GitLoader();
	git_loader->addRepository("/home/sasha/kde-ru/kde-ru-trunk.git/.git");
	git_loader->addRepository("/home/sasha/kde-ru/kde-l10n-ru-stable/.git");

	for (int i = 1; i < argc; i ++)
	{
		TranslationContent *new_content = new TranslationContent(argv[i]);

		const git_oid *tp_hash = new_content->calculateTpHash();
		if (!tp_hash) // not a .po file
			continue;

		TranslationContent *old_content = git_loader->findOldestByTphash(tp_hash);
		if (!old_content)
		{
			printf("Could not find old content for [%s]\n", argv[i]);
			assert(0);
		}

		std::cout << Iddiffer::generateIddiffText(old_content, new_content);
	}

	return 0;
}

