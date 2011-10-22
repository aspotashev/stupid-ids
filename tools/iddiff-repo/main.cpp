
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

std::string path_to_basename(std::string path)
{
    size_t pos = path.rfind('/');
    if (pos == std::string::npos) // no '/' in the path
        return path;
    else
        return path.substr(pos + 1);
}

void processFile(GitLoader *git_loader, Iddiffer *merged_diff, const char *filename)
{
    TranslationContent *input_content = new TranslationContent(filename);

    const git_oid *tp_hash = input_content->calculateTpHash();
    if (!tp_hash) // not a .po file
        return;

    int first_id = stupidsClient.getFirstId(tp_hash);
    if (first_id == -1) // couldn't connect to the server
        assert(0);
    else if (first_id == 0)
    {
        // TODO: (kind of?) run mergemsg against the official
        // .pot template at "lower_bound(<date of current .po>)"
        // with the same file basename as the given .po file
        // (dolphin.po -> dolphin.pot).

        if (globalArgs.branch &&
            strcmp(globalArgs.branch, "trunk") != 0 &&
            strcmp(globalArgs.branch, "stable") != 0)
        {
            globalArgs.branch = NULL;

            fprintf(stderr,
                    "Unknown value for option \'branch\': %s\n"
                    "Must be \'trunk\' or \'stable\'\n", globalArgs.branch);
            return;
        }

        if (globalArgs.branch == NULL)
        {
            fprintf(stderr,
                    "Branch (\'trunk\' or \'stable\') required, but not given\n");
            return;
        }

        assert(strcmp(globalArgs.branch, "stable") == 0); // FIXME (temporary hack)

        Repository *repo = new Repository("/home/sasha/kde-ru/xx-numbering/stable-templates/.git"); // FIXME (temporary hack)
        const git_oid *best_pot_oid = repo->findRelevantPot((path_to_basename(filename) + "t").c_str(), input_content->potDate());
        if (!best_pot_oid)
        {
            fprintf(stderr,
                    "Unknown tp_hash. Translation file: [%s], basename: [%s]\n"
                    "This probably means that someone (probably, translator of this file) "
                    "has generated the translation template by hand, or has changed "
                    "something in the template.\n",
                    filename, (path_to_basename(filename) + "t").c_str());
            assert(0);
        }

        TranslationContent *pot_content = new TranslationContent(repo, best_pot_oid);
        pot_content->setDisplayFilename(filename);
        tp_hash = pot_content->calculateTpHash();

        //printf("filename=%s, oid=%s, tphash=%s\n", filename, GitOid(best_pot_oid).toString().c_str(), GitOid(tp_hash).toString().c_str());

        first_id = stupidsClient.getFirstId(tp_hash);
        if (first_id == -1)
            assert(0);
        if (first_id == 0)
            assert(0); // you need to update the first_id database

        // Fill "pot_content" with translations from "input_content"
        pot_content->copyTranslationsFrom(input_content);
        pot_content->setAuthor(""); // TODO: class TranslationContent should automatically remove the author name if it is "FULL NAME <EMAIL@ADDRESS>"
        input_content = pot_content;
    }

    Iddiffer *diff = new Iddiffer();

    // If there are more than one .po with the given tp_hash in the
    // official repositories, we should compare against the oldest
    // .po file, because ... (So, why? -- Because some info may be lost?)
    TranslationContent *old_content = git_loader->findOldestByTphash(tp_hash);
    old_content->setDisplayFilename(filename);
    if (old_content)
        diff->diffFiles(old_content, input_content);
    else
        diff->diffAgainstEmpty(input_content); // diff against /dev/null

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
        switch (opt)
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

