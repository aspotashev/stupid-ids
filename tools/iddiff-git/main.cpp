
#include <assert.h>
#include <stdio.h>
#include <iostream>

#include <gettextpo-helper/translationcontent.h>
#include <gettextpo-helper/iddiff.h>
#include <gettextpo-helper/gitloader.h>


int main(int argc, char *argv[])
{
	if (argc != 3) // 2 arguments
	{
		fprintf(stderr, "Example usage:\n\tiddiff-git ./.git 7fb8df3aed979214165e7c7d28e672966b13a15b\n");
		exit(1);
	}

	Repository *repo = new Repository(argv[1]);
	const char *input_oid_str = argv[2];
	repo->readRepositoryCommits();

	git_oid input_oid;
    // TBD: avoid usage of libgit2 directly, better use gettextpo-helper
	assert(git_oid_fromstr(&input_oid, input_oid_str) == 0);

	int commit_index = repo->commitIndexByOid(&input_oid);
	if (commit_index < 0)
	{
		printf("No commit with OID %s in the Git repository at %s\n", input_oid_str, repo->gitDir());
		assert(0);
	}

	const Commit *commit = repo->commit(commit_index);
	Iddiffer *merged_diff = new Iddiffer();
	for (int i = 0; i < commit->nChanges(); i ++)
	{
		const CommitFileChange *change = commit->change(i);
		Iddiffer *diff = new Iddiffer();

		if (change->type() == CommitFileChange::DEL)
			continue;

		TranslationContent *new_content = new TranslationContent(repo, change->oid2());
		TranslationContent *old_content = NULL;
		if (change->type() == CommitFileChange::ADD)
		{
			diff->diffAgainstEmpty(new_content);
		}
		else if (change->type() == CommitFileChange::MOD)
		{
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

