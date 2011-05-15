
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <vector>

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

// TODO: use common code with gettextpo-helper/lib/gettextpo-helper.cpp
char *xstrdup(const char *str)
{
	size_t len = strlen(str);
	char *dup = new char [len + 1];
	strcpy(dup, str);

	return dup;
}

class CommitFileChange;

class Commit
{
public:
	Commit();
	~Commit();

	void addChange(CommitFileChange *change);

public:
	std::vector<CommitFileChange *> m_changes;
};

class CommitFileChange
{
public:
	CommitFileChange(
		const git_oid *oid1, const git_oid *oid2,
		const char *path, const char *name, int type);
	~CommitFileChange();

	void print();

public:
	git_oid m_oid1;
	git_oid m_oid2;
	char *m_path;
	char *m_name;
	int m_type;

	enum
	{
		ADD = 1,
		DEL = 2,
		MOD = 3
	};
};

Commit::Commit()
{
}

Commit::~Commit()
{
	for (size_t i = 0; i < m_changes.size(); i ++)
		delete m_changes[i];
}

void Commit::addChange(CommitFileChange *change)
{
	m_changes.push_back(change);
}

CommitFileChange::CommitFileChange(
	const git_oid *oid1, const git_oid *oid2,
	const char *path, const char *name, int type)
{
	switch (type)
	{
	case ADD:
		assert(oid1 == NULL);
		assert(oid2 != NULL);
		break;
	case DEL:
		assert(oid1 != NULL);
		assert(oid2 == NULL);
		break;
	case MOD:
		assert(oid1 != NULL);
		assert(oid2 != NULL);
		break;
	default:
		assert(0);
	}

	if (oid1)
		git_oid_cpy(&m_oid1, oid1);
	if (oid2)
		git_oid_cpy(&m_oid2, oid2);
	m_path = xstrdup(path);
	m_name = xstrdup(name);
	m_type = type;
}

CommitFileChange::~CommitFileChange()
{
	delete [] m_path;
	delete [] m_name;
}

void CommitFileChange::print()
{
	switch (m_type)
	{
	case ADD:
		printf("A    (%s) %s\n", m_path, m_name); // a file was added
		break;
	case DEL:
		printf("D    (%s) %s\n", m_path, m_name); // a file was removed
		break;
	case MOD:
		printf("M    (%s) %s\n", m_path, m_name); // a file was modified
		break;
	default:
		assert(0);
	}
}

class Repository
{
public:
	Repository(const char *git_dir);
	~Repository();

	void diffTree(git_tree *tree1, git_tree *tree2, const char *path);
	void diffCommit(git_commit *commit1, git_commit *commit2);
	void readRepository(const char *git_dir);

protected:
	git_tree *git_tree_entry_subtree(const git_tree_entry *entry);

private:
	git_repository *repo;
	const git_oid *oid_master;

	Commit *m_currentCommit;
	std::vector<Commit *> m_commits;
};

Repository::Repository(const char *git_dir)
{
	readRepository(git_dir);
}

Repository::~Repository()
{
	for (size_t i = 0; i < m_commits.size(); i ++)
		delete m_commits[i];
}

git_tree *Repository::git_tree_entry_subtree(const git_tree_entry *entry)
{
	const git_oid *subtree_oid = git_tree_entry_id(entry);
	git_tree *subtree;
	assert(git_tree_lookup(&subtree, repo, subtree_oid) == 0);

	return subtree;
}

char *concat_path(const char *path, const char *name)
{
	char *res = new char [strlen(path) + strlen(name) + 2];
	strcpy(res, path);
	strcat(res, "/");
	strcat(res, name);

	return res;
}

void Repository::diffTree(git_tree *tree1, git_tree *tree2, const char *path)
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
			const char *name = git_tree_entry_name(entry1);
			if (git_tree_entry_attributes(entry1) & REPO_MODE_DIR)
			{
				git_tree *subtree1 = git_tree_entry_subtree(entry1);
				char *subtree_path = concat_path(path, name);

				diffTree(subtree1, NULL, subtree_path);
				delete [] subtree_path;
				git_tree_close(subtree1);
			}
			else
			{
				CommitFileChange *change = new CommitFileChange(
					git_tree_entry_id(entry1), NULL, path, name, CommitFileChange::DEL);
				m_currentCommit->addChange(change);
				change->print();
			}

			next1 = true;
		}
		else if (idx1 >= count1 || cmp > 0) // entry2 goes first, i.e. the entry is being added in this commit
		{
			const char *name = git_tree_entry_name(entry2);
			if (git_tree_entry_attributes(entry2) & REPO_MODE_DIR)
			{
				git_tree *subtree2 = git_tree_entry_subtree(entry2);
				char *subtree_path = concat_path(path, name);

				diffTree(NULL, subtree2, subtree_path);
				delete [] subtree_path;
				git_tree_close(subtree2);
			}
			else
			{
				CommitFileChange *change = new CommitFileChange(
					NULL, git_tree_entry_id(entry2), path, name, CommitFileChange::ADD);
				m_currentCommit->addChange(change);
				change->print();
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

				const char *name = git_tree_entry_name(entry1);
				if (git_tree_entry_attributes(entry1) & REPO_MODE_DIR) // tree
				{
					git_tree *subtree1 = git_tree_entry_subtree(entry1);
					git_tree *subtree2 = git_tree_entry_subtree(entry2);
					char *subtree_path = concat_path(path, name);

					diffTree(subtree1, subtree2, subtree_path);
					delete [] subtree_path;
					git_tree_close(subtree1);
					git_tree_close(subtree2);
				}
				else // blob
				{
					CommitFileChange *change = new CommitFileChange(
						git_tree_entry_id(entry1), git_tree_entry_id(entry2), path, name, CommitFileChange::MOD);
					m_currentCommit->addChange(change);
					change->print();
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
}

void Repository::diffCommit(git_commit *commit1, git_commit *commit2)
{
	git_tree *tree1 = NULL, *tree2 = NULL;
	if (commit1)
		assert(git_commit_tree(&tree1, commit1) == 0);
	if (commit2)
		assert(git_commit_tree(&tree2, commit2) == 0);

	m_currentCommit = new Commit();

	diffTree(tree1, tree2, "");

	m_commits.push_back(m_currentCommit);

	if (tree1)
		git_tree_close(tree1);
	if (tree2)
		git_tree_close(tree2);
}

void Repository::readRepository(const char *git_dir)
{
	// Open repository
	assert(git_repository_open(&repo, git_dir) == 0);

	git_reference *ref_master;
	assert(git_reference_lookup(&ref_master, repo, "refs/heads/master") == 0);

	oid_master = git_reference_oid(ref_master);
	assert(oid_master != NULL);

	// Read repository
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

		git_commit_close(commit);
		commit = parent;
	}

	// Close repository
	git_repository_free(repo);
	repo = NULL;
	oid_master = NULL;
}

//--------------------------------

int main()
{
	Repository *repo = new Repository("/home/sasha/kde-ru/xx-numbering/templates/.git/");
	Repository *repo_stable = new Repository("/home/sasha/kde-ru/xx-numbering/stable-templates/.git/");

	// ...
	// ... run detectors ...
	// ...

	delete repo;
	delete repo_stable;

	return 0;
}

