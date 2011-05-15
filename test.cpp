
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <git2.h>

#define REPO_MODE_DIR 040000

int git_tree_entry_namecmp(const git_tree_entry *entry1, const git_tree_entry *entry2)
{
	// TODO: optimize this
	static char name1[5000];
	static char name2[5000];
	strcpy(name1, git_tree_entry_name(entry1));
	strcpy(name2, git_tree_entry_name(entry2));
	if (git_tree_entry_attributes(entry1) & REPO_MODE_DIR)
		strcat(name1, "/");
	if (git_tree_entry_attributes(entry2) & REPO_MODE_DIR)
		strcat(name2, "/");

	return strcmp(name1, name2);
//	return strcmp(git_tree_entry_name(entry1), git_tree_entry_name(entry2));
}

class Repository
{
public:
	Repository(const char *git_dir);
	~Repository();

	void diffTree(git_tree *tree1, git_tree *tree2);
	void diffCommit(git_commit *commit1, git_commit *commit2);
	void run();

protected:
	git_tree *git_tree_entry_subtree(const git_tree_entry *entry);

protected:
	git_repository *repo;
	const git_oid *oid_master;
};

Repository::Repository(const char *git_dir)
{
	assert(git_repository_open(&repo, git_dir) == 0);

	git_reference *ref_master;
	assert(git_reference_lookup(&ref_master, repo, "refs/heads/master") == 0);

	oid_master = git_reference_oid(ref_master);
	assert(oid_master != NULL);
}

Repository::~Repository()
{
	git_repository_free(repo);
}

git_tree *Repository::git_tree_entry_subtree(const git_tree_entry *entry)
{
	const git_oid *subtree_oid = git_tree_entry_id(entry);
	git_tree *subtree;
	assert(git_tree_lookup(&subtree, repo, subtree_oid) == 0);

	return subtree;
}

void Repository::diffTree(git_tree *tree1, git_tree *tree2)
{
	// This should have been checked earlier
	if (tree1 != NULL && tree2 != NULL && git_oid_cmp(git_tree_id(tree1), git_tree_id(tree2)) == 0)
	{
		printf("Empty commit!\n");
		return;
	}

	// NOTE: the trees will be read twice for every commit, because
	// almost every commit is a parent and is a child at the same time.
	size_t count1 = tree1 ? git_tree_entrycount(tree1) : 0;
	size_t count2 = tree2 ? git_tree_entrycount(tree2) : 0;
	size_t idx1 = 0;
	size_t idx2 = 0;

	const git_tree_entry *entry1 = NULL;
	if (count1 > 0)
		entry1 = git_tree_entry_byindex(tree1, 0);
	const git_tree_entry *entry2 = NULL;
	if (count2 > 0)
		entry2 = git_tree_entry_byindex(tree2, 0);

	while (idx1 < count1 || idx2 < count2)
	{
		// We assume that tree entries are sorted by name
		if (count1 > 0 && idx1 < count1 - 1)
			assert(git_tree_entry_namecmp(git_tree_entry_byindex(tree1, idx1), git_tree_entry_byindex(tree1, idx1 + 1)) < 0);
		if (count2 > 0 && idx2 < count2 - 1)
			assert(git_tree_entry_namecmp(git_tree_entry_byindex(tree2, idx2), git_tree_entry_byindex(tree2, idx2 + 1)) < 0);
		//-----------------------------------------------

		bool next1 = false;
		bool next2 = false;

		int cmp = 0;
		if (idx1 < count1 && idx2 < count2) // compare only if there are entries to compare
			cmp = strcmp(git_tree_entry_name(entry1), git_tree_entry_name(entry2));

		if (idx2 >= count2 || cmp < 0) // entry1 goes first, i.e. the entry is being removed in this commit
		{
			if (git_tree_entry_attributes(entry1) & REPO_MODE_DIR)
			{
				git_tree *subtree1 = git_tree_entry_subtree(entry1);
				diffTree(subtree1, NULL);
			}
			else
			{
				printf("D    %s\n", git_tree_entry_name(entry1)); // a file was removed
			}

			next1 = true;
		}
		else if (idx1 >= count1 || cmp > 0) // entry2 goes first, i.e. the entry is being added in this commit
		{
			if (git_tree_entry_attributes(entry2) & REPO_MODE_DIR)
			{
				git_tree *subtree2 = git_tree_entry_subtree(entry2);
				diffTree(NULL, subtree2);
			}
			else
			{
				printf("A    %s\n", git_tree_entry_name(entry2)); // a file was added
			}

			next2 = true;
		}
		else // both entries exist and have the same names, i.e. the entry is being modified in this commit
		{
			if (git_oid_cmp(git_tree_entry_id(entry1), git_tree_entry_id(entry2)) == 0)
			{
//				printf("Entry unchanged.\n");
			}
			else
			{
				assert(git_tree_entry_attributes(entry1) == git_tree_entry_attributes(entry2));

				unsigned int attr = git_tree_entry_attributes(entry1);
				if (attr & REPO_MODE_DIR) // tree
				{
					git_tree *subtree1 = git_tree_entry_subtree(entry1);
					git_tree *subtree2 = git_tree_entry_subtree(entry2);

					// Run 'diffTree' recursively
					diffTree(subtree1, subtree2);
				}
				else // blob
				{
//					printf("M    %s\n", git_tree_entry_name(entry1)); // a file was modified
				}
			}

			next1 = next2 = true;
		}

		if (next1)
		{
			idx1 ++;
			if (idx1 < count1)
				entry1 = git_tree_entry_byindex(tree1, idx1);
		}
		if (next2)
		{
			idx2 ++;
			if (idx2 < count2)
				entry2 = git_tree_entry_byindex(tree2, idx2);
		}
	}


/*	for (;;)
	{
		const git_tree_entry *entry = git_tree_entry_byindex(tree, i);
		unsigned int attr = git_tree_entry_attributes(entry);
		printf("\t[%s], %o\n",
			git_tree_entry_name(entry),
			attr);

		git_object *entry_object;
		git_tree_entry_2object(&entry_object, repo, entry);

		if (attr & REPO_MODE_DIR)
		{
			assert(git_object_type(entry_object) == GIT_OBJ_TREE);

			const git_oid *subtree_oid = git_object_id(entry_object);
			git_tree *subtree;
			assert(git_tree_lookup(&subtree, repo, subtree_oid) == 0);

			runTree(subtree);
		}
		else
		{
			printf("Attr: %o\n", attr);
			printf("Git object type: %d\n", git_object_type(entry_object));
			assert(git_object_type(entry_object) == GIT_OBJ_BLOB);
			assert(0);
		}
	}*/
}

void Repository::diffCommit(git_commit *commit1, git_commit *commit2)
{
	git_tree *tree1 = NULL, *tree2 = NULL;
	if (commit1)
		assert(git_commit_tree(&tree1, commit1) == 0);
	if (commit2)
		assert(git_commit_tree(&tree2, commit2) == 0);

	diffTree(tree1, tree2);
}

void Repository::run()
{
	git_commit *commit;
	git_commit *parent;
	assert(git_commit_lookup(&commit, repo, oid_master) == 0);

	while (commit != NULL)
	{
		char oid_string[GIT_OID_HEXSZ + 1];
		git_oid_to_string(oid_string, GIT_OID_HEXSZ + 1, git_commit_id(commit));
		printf("Current commit's oid: %s\n", oid_string);

		// Moving to parent
		unsigned int parentcount = git_commit_parentcount(commit);
		assert (parentcount <= 1);

		if (parentcount == 1)
			git_commit_parent(&parent, commit, 0);
		else
			parent = NULL; // root commit

		// Calculate changes made by this commit
		diffCommit(parent, commit);

		commit = parent;
	}
}

//--------------------------------

int main()
{
	Repository *repo = new Repository("/home/sasha/kde-ru/xx-numbering/templates/.git/");
	repo->run();
	delete repo;

	return 0;
}

