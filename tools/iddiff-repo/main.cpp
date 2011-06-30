
#include <assert.h>
#include <stdio.h>
#include <iostream>

#include <gettextpo-helper/translationcontent.h>
#include <gettextpo-helper/iddiff.h>
#include <gettextpo-helper/gitloader.h>
#include <gettextpo-helper/stupids-client.h>


void processFile(GitLoader *git_loader, Iddiffer *merged_diff, const char *filename)
{
	TranslationContent *new_content = new TranslationContent(filename);

	const git_oid *tp_hash = new_content->calculateTpHash();
	if (!tp_hash) // not a .po file
		return;
	if (stupidsClient.getFirstId(tp_hash) == 0)
	{
		printf("Unknown tp_hash. Translation file: [%s]\nThis probably means that someone (probably, translator of this file) has generated the translation template by hand, or has changed something in the template.\n", filename);
		return;
	}

	Iddiffer *diff = new Iddiffer();
	TranslationContent *old_content = git_loader->findOldestByTphash(tp_hash);
	if (old_content)
		diff->diffFiles(old_content, new_content);
	else
		diff->diffAgainstEmpty(new_content); // diff against /dev/null

	merged_diff->merge(diff);
	delete diff;
}

int main(int argc, char *argv[])
{
	// For loading blobs by their OIDs
	GitLoader *git_loader = new GitLoader();
	git_loader->addRepository("/home/sasha/kde-ru/kde-ru-trunk.git/.git");
	git_loader->addRepository("/home/sasha/kde-ru/kde-l10n-ru-stable/.git");

	Iddiffer *merged_diff = new Iddiffer();

	for (int i = 1; i < argc; i ++)
		processFile(git_loader, merged_diff, argv[i]);

	std::cout << merged_diff->generateIddiffText();

	return 0;
}

