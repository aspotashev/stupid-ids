
#include <assert.h>
#include <stdio.h>
#include <iostream>

#include <gettextpo-helper/translationcontent.h>
#include <gettextpo-helper/iddiff.h>
#include <gettextpo-helper/gitloader.h>
#include <gettextpo-helper/stupids-client.h>

struct
{
	const char *branch;

	int numInputFiles;
	char **inputFiles;
} static globalArgs;

void processFile(GitLoader *git_loader, Iddiffer *merged_diff, const char *filename)
{
	TranslationContent *new_content = new TranslationContent(filename);

	const git_oid *tp_hash = new_content->calculateTpHash();
	if (!tp_hash) // not a .po file
		return;
	if (stupidsClient.getFirstId(tp_hash) == 0)
	{
		// TODO: (kind of?) run mergemsg against the official
		// .pot template at "lower_bound(<date of current .po>)"
		// with the same file basename as the given .po file
		// (dolphin.po -> dolphin.pot).

		fprintf(stderr,
			"Unknown tp_hash. Translation file: [%s]\n"
			"This probably means that someone (probably, translator of this file) "
			"has generated the translation template by hand, or has changed "
			"something in the template.\n", filename);
		return;
	}

	Iddiffer *diff = new Iddiffer();

	// If there are more than one .po with the given tp_hash in the
	// official repositories, we should compare against the oldest
	// .po file, because ... (So, why? -- Because some info may be lost?)
	TranslationContent *old_content = git_loader->findOldestByTphash(tp_hash);
	if (old_content)
		diff->diffFiles(old_content, new_content);
	else
		diff->diffAgainstEmpty(new_content); // diff against /dev/null

	merged_diff->merge(diff);
	delete diff;
}

// Creates an "iddiff" for all given .po files against respective .po
// files in the official translations repository.
int main(int argc, char *argv[])
{
	// List of options:
	//     b <branch> -- "trunk" or "stable", necessary when tp_hash was not found.
	const char opt_string[] = "b:";

	// Initialize globalArgs
	globalArgs.branch = NULL;

	int opt = 0;
	while ((opt = getopt(argc, argv, opt_string)) != -1)
	{
		switch(opt)
		{
		case 'b':
			globalArgs.branch = optarg;
			break;
		default:
			assert(0);
			break;
		}
	}

	globalArgs.numInputFiles = argc - optind;
	globalArgs.inputFiles = argv + optind;

	if (globalArgs.numInputFiles == 0)
	{
		fprintf(stderr, "Usage: iddiff-repo [file1.po] [file2.po] [...] [fileN.po]\n"
			"\n"
			"At least one .po file should be given.\n");
		return 1;
	}

	// For loading blobs by their OIDs
	GitLoader *git_loader = new GitLoader();
	git_loader->addRepository("/home/sasha/kde-ru/kde-ru-trunk.git/.git");
	git_loader->addRepository("/home/sasha/kde-ru/kde-l10n-ru-stable/.git");

	Iddiffer *merged_diff = new Iddiffer();

	for (int i = 0; i < globalArgs.numInputFiles; i ++)
		processFile(git_loader, merged_diff, globalArgs.inputFiles[i]);

	std::cout << merged_diff->generateIddiffText();

	return 0;
}

