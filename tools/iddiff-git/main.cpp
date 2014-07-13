#include <gtpo/translationcontent.h>
#include <gtpo/iddiff.h>
#include <gtpo/repository.h>
#include <gtpo/gettextpo-helper.h>
#include <gtpo/commit.h>
#include <gtpo/commitfilechange.h>

#include <iostream>

int toolIddiffGit(int argc, char *argv[])
{
    const std::vector<std::string>& args = parseArgs(argc, argv);

    if (args.size() != 2) {
        fprintf(stderr, "Example usage:\n\tiddiff-git ./.git 7fb8df3aed979214165e7c7d28e672966b13a15b\n");
        exit(1);
    }

    Repository* repo = new Repository(args[0]);
    std::string input_oid_str = args[1];
    repo->readRepositoryCommits();

    git_oid input_oid;
    // TBD: avoid usage of libgit2 directly, better use gettextpo-helper
    assert(git_oid_fromstr(&input_oid, input_oid_str.c_str()) == 0);

    int commit_index = repo->commitIndexByOid(&input_oid);
    if (commit_index < 0) {
        printf("No commit with OID %s in the Git repository at %s\n",
               input_oid_str.c_str(), repo->gitDir().c_str());
        assert(0);
    }

    const Commit* commit = repo->commit(commit_index);
    Iddiffer* merged_diff = new Iddiffer();
    for (int i = 0; i < commit->nChanges(); i ++) {
        const CommitFileChange *change = commit->change(i);
        Iddiffer *diff = new Iddiffer();

        if (change->type() == CommitFileChange::DEL)
            continue;

        TranslationContent *new_content = new TranslationContent(repo, change->oid2());
        TranslationContent *old_content = NULL;
        if (change->type() == CommitFileChange::ADD) {
            diff->diffAgainstEmpty(new_content);
        }
        else if (change->type() == CommitFileChange::MOD) {
            old_content = new TranslationContent(repo, change->oid1());
            diff->diffFiles(old_content, new_content);
        }
        else assert(0);

        delete new_content;
        if (old_content)
            delete old_content;

        merged_diff->merge(diff);
        delete diff;
    }

    // TODO: copy commit->time() ("Date:"), author ("Author:") and message ("Subject:") to the header of Iddiff

    std::cout << merged_diff->generateIddiffText();

    return 0;
}
