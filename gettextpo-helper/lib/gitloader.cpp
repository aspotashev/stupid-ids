
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <algorithm>

#include <git2.h>

#include <gettextpo-helper/gitloader.h>
#include <gettextpo-helper/gettextpo-helper.h>

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

//----------------------------------------

Commit::Commit(git_commit *commit)
{
	// TODO: how to handle git_commit_time_offset?
	m_time = git_commit_time(commit);
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

int Commit::nChanges() const
{
	return (int)m_changes.size();
}

const CommitFileChange *Commit::change(int index) const
{
	assert(index >= 0 && index < nChanges());

	return m_changes[index];
}

git_time_t Commit::time() const
{
	return m_time;
}

//----------------------------------------

int CommitFileChange::type() const
{
	return m_type;
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
	m_path = xstrdup(path + (path[0] == '/' ? 1 : 0));
	m_name = xstrdup(name);
	m_type = type;
}

CommitFileChange::~CommitFileChange()
{
	delete [] m_path;
	delete [] m_name;
}

const git_oid *CommitFileChange::oid1() const
{
	assert(m_type == DEL || m_type == MOD);

	return &m_oid1;
}

const git_oid *CommitFileChange::oid2() const
{
	assert(m_type == ADD || m_type == MOD);

	return &m_oid2;
}

const git_oid *CommitFileChange::tryOid1() const
{
	if (m_type == DEL || m_type == MOD)
		return &m_oid1;
	else
		return NULL;
}

const git_oid *CommitFileChange::tryOid2() const
{
	if (m_type == ADD || m_type == MOD)
		return &m_oid2;
	else
		return NULL;
}

const char *CommitFileChange::path() const
{
	return m_path;
}

const char *CommitFileChange::name() const
{
	return m_name;
}

void CommitFileChange::print() const
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

void build_pathname(char *dest, const char *path, const char *name)
{
	strcpy(dest, path);
	strcat(dest, "/");
	strcat(dest, name);
}

const CommitFileChange *Commit::findChange(const char *name, const char *path) const
{
	// Using binary search (the items of m_changes are sorted by path+'/'+name)
	int left = 0;
	int right = nChanges() - 1;

	static char pattern_fullname[5000];
	static char fullname[5000];

	build_pathname(pattern_fullname, path, name);
	while (left <= right)
	{
		int mid = (left + right) / 2;
		const CommitFileChange *change = this->change(mid);
		build_pathname(fullname, change->path(), change->name());

		int cmp = strcmp(pattern_fullname, fullname);
		if (cmp > 0)
			left = mid + 1;
		else if (cmp < 0)
			right = mid - 1;
		else
			return change;
	}

	return NULL;
}

const git_oid *Commit::findRemovalOid(const char *name, const char *path) const
{
	const CommitFileChange *change = findChange(name, path);
	if (change && change->type() == CommitFileChange::DEL)
		return change->oid1();
	else
		return NULL;
}

// Finds addition or modification
const git_oid *Commit::findUpdateOid(const char *name, const char *path) const
{
	const CommitFileChange *change = findChange(name, path);
	if (change &&
		(change->type() == CommitFileChange::ADD ||
		change->type() == CommitFileChange::MOD))
		return change->oid2();
	else
		return NULL;
}

//---------------------------------------

Repository::Repository(const char *git_dir)
{
	m_gitDir = xstrdup(git_dir);

	m_repo = NULL;
	m_commitsInit = false;
}

Repository::~Repository()
{
	delete [] m_gitDir;

	for (size_t i = 0; i < m_commits.size(); i ++)
		delete m_commits[i];
}

git_tree *Repository::git_tree_entry_subtree(const git_tree_entry *entry)
{
	assert(m_repo);

	const git_oid *subtree_oid = git_tree_entry_id(entry);
	git_tree *subtree;
	assert(git_tree_lookup(&subtree, m_repo, subtree_oid) == 0);

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

void Repository::diffTree(git_tree *tree1, git_tree *tree2, const char *path, Commit *currentCommit)
{
	// This should have been checked earlier
	if (tree1 != NULL && tree2 != NULL && git_oid_cmp(git_tree_id(tree1), git_tree_id(tree2)) == 0)
	{
//		printf("Empty commit!\n");
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
			cmp = git_tree_entry_namecmp(entry1, entry2);

		if (idx2 >= count2 || cmp < 0) // entry1 goes first, i.e. the entry is being removed in this commit
		{
			const char *name = git_tree_entry_name(entry1);
			if (git_tree_entry_attributes(entry1) & REPO_MODE_DIR)
			{
				git_tree *subtree1 = git_tree_entry_subtree(entry1);
				char *subtree_path = concat_path(path, name);

				diffTree(subtree1, NULL, subtree_path, currentCommit);
				delete [] subtree_path;
				git_tree_close(subtree1);
			}
			else
			{
				CommitFileChange *change = new CommitFileChange(
					git_tree_entry_id(entry1), NULL, path, name, CommitFileChange::DEL);
				currentCommit->addChange(change);
//				change->print();
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

				diffTree(NULL, subtree2, subtree_path, currentCommit);
				delete [] subtree_path;
				git_tree_close(subtree2);
			}
			else
			{
				CommitFileChange *change = new CommitFileChange(
					NULL, git_tree_entry_id(entry2), path, name, CommitFileChange::ADD);
				currentCommit->addChange(change);
//				change->print();
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

					diffTree(subtree1, subtree2, subtree_path, currentCommit);
					delete [] subtree_path;
					git_tree_close(subtree1);
					git_tree_close(subtree2);
				}
				else // blob
				{
					CommitFileChange *change = new CommitFileChange(
						git_tree_entry_id(entry1), git_tree_entry_id(entry2), path, name, CommitFileChange::MOD);
					currentCommit->addChange(change);
//					change->print();
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

// commit1 -- parent commit
// commit2 -- current commit
void Repository::diffCommit(git_commit *commit1, git_commit *commit2)
{
	// This function should be called only during initialization of "m_commits".
	assert(!m_commitsInit);

	git_tree *tree1 = NULL, *tree2 = NULL;
	if (commit1)
		assert(git_commit_tree(&tree1, commit1) == 0);
	if (commit2)
		assert(git_commit_tree(&tree2, commit2) == 0);

	Commit *currentCommit = new Commit(commit2);

	diffTree(tree1, tree2, "", currentCommit);

	m_commits.push_back(currentCommit);

	if (tree1)
		git_tree_close(tree1);
	if (tree2)
		git_tree_close(tree2);
}

void Repository::readRepositoryCommits()
{
	assert(m_repo == NULL);
	if (m_commitsInit)
		return;

	// Open repository
	assert(git_repository_open(&m_repo, m_gitDir) == 0);

	git_reference *ref_master;
	assert(git_reference_lookup(&ref_master, m_repo, "refs/heads/master") == 0);

	const git_oid *oid_master = git_reference_oid(ref_master);
	assert(oid_master != NULL);

	// Read repository
	git_commit *commit;
	git_commit *parent;
	assert(git_commit_lookup(&commit, m_repo, oid_master) == 0);

	while (commit != NULL)
	{
		char oid_string[GIT_OID_HEXSZ + 1];
		git_oid_to_string(oid_string, GIT_OID_HEXSZ + 1, git_commit_id(commit));
//		printf("Current commit's oid: %s\n", oid_string);

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
	git_repository_free(m_repo);
	m_repo = NULL;

	// Reverse commit list (to set the order from root to HEAD)
	reverse(m_commits.begin(), m_commits.end());

	for (size_t i = 0; i < m_commits.size() - 1; i ++)
		if (m_commits[i]->time() > m_commits[i + 1]->time() + 394)
		{
			// WARNING: it turns out that commits do no
			// always have increasing time.

			// There was something strange with the following commits:
			//   http://websvn.kde.org/?view=revision&revision=1137453
			//   http://websvn.kde.org/?view=revision&revision=1137452

			printf("m_commits[i]->time = %lld, m_commits[i + 1]->time = %lld, i = %d\n",
				(long long int)m_commits[i]->time(),
				(long long int)m_commits[i + 1]->time(),
				(int)i);
			assert(0);
		}

	// Going to the past of two commits is too much ;)
	// (if there wouldn't be such a limit, the binary
	// search by time may _completely_ fail)
	for (size_t i = 0; i < m_commits.size() - 2; i ++)
		if (m_commits[i]->time() >= m_commits[i + 2]->time())
			assert(0);

	m_commitsInit = true;
}

int Repository::nCommits() const
{
	assert(m_commitsInit);

	return (int)m_commits.size();
}

const Commit *Repository::commit(int index) const
{
	assert(m_commitsInit);
	assert(index >= 0 && index < nCommits());

	return m_commits[index];
}

const git_oid *Repository::findLastRemovalOid(int from_commit, const char *name, const char *path) const
{
	// If from_commit < 0, this functios gratefully returns NULL
	assert(from_commit < nCommits());

	// Reverse search in all commits older than from_commit
	for (int i = from_commit; i >= 0 && i >= from_commit - 10; i --) // FIXME: remove "&& i >= from_commit - 10" when findRemovalOid becomes fast
	{
		const git_oid *oid = commit(i)->findRemovalOid(name, path);
		if (oid)
			return oid;
	}

	return NULL;
}

const git_oid *Repository::findNextUpdateOid(int from_commit, const char *name, const char *path) const
{
	assert(from_commit >= 0 && from_commit < nCommits());

	// Forward search in all commits newer than from_commit
	for (int i = from_commit; i < nCommits(); i ++)
	{
		const git_oid *oid = commit(i)->findUpdateOid(name, path);
		if (oid)
			return oid;
	}

	return NULL;
}

const git_oid *Repository::findLastUpdateOid(int from_commit, const char *name, const char *path) const
{
	assert(from_commit >= 0 && from_commit < nCommits());

	// Forward search in all commits newer than from_commit
	for (int i = from_commit; i >= 0; i --)
	{
		const git_oid *oid = commit(i)->findUpdateOid(name, path);
		if (oid)
			return oid;
	}

	return NULL;
}

// Return the latest commit done before the given time.
int Repository::lastCommitByTime(git_time_t time) const
{
	if (nCommits() == 0 || time < commit(0)->time())
		return -1; // No such commit

	int l = 0;
	int r = nCommits() - 1;

	// Using "loose" binary search, because commits
	// are not strictly sorted by time.
	while (r - l > 2)
	{
		int m = (l + r) / 2;
		if (time < commit(m)->time())
			r = m;
		else
			l = m;
	}

	for (; r >= l; r --)
		if (time >= commit(r)->time())
			return r;

	assert(0); // Could not find the commit, but it must be between 'l' and 'r'
	return -1;
}

// Returns the OID of the file if it exists at the given time.
const git_oid *Repository::findFileOidByTime(git_time_t time, const char *name, const char *path) const
{
	int commit_index = lastCommitByTime(time);
	if (commit_index == -1)
		return NULL;

	for (; commit_index >= 0; commit_index --)
	{
		const git_oid *oid = commit(commit_index)->findUpdateOid(name, path);
		if (oid)
			return oid;
	}

	return NULL;
}

void Repository::dumpOids(std::vector<GitOid2Change> &dest) const
{
	for (int i = 0; i < nCommits(); i ++)
		for (int j = 0; j < commit(i)->nChanges(); j ++)
		{
			const CommitFileChange *change = commit(i)->change(j);

			const git_oid *oid;
			oid = change->tryOid1();
			if (oid)
				dest.push_back(GitOid2Change(oid, change));
			oid = change->tryOid2();
			if (oid)
				dest.push_back(GitOid2Change(oid, change));
		}
}

//---------------------------------------------------------

GitLoader::GitLoader()
{
}

GitLoader::~GitLoader()
{
	for (size_t i = 0; i < m_repos.size(); i ++)
	{
		git_repository_free(m_repos[i]);
		m_repos[i] = NULL;
	}
}

/**
 * \brief Search blob by OID in all repositories added using addRepository().
 *
 * \param oid Git object ID of the blob.
 *
 * \returns Blob or NULL, if it was not found in any of the repositories.
 * It is necessary to call this method when you stop using a blob. Failure to do so will cause a memory leak.
 */
git_blob *GitLoader::blobLookup(const git_oid *oid)
{
	git_blob *blob;

	for (size_t i = 0; i < m_repos.size(); i ++)
		if (git_blob_lookup(&blob, m_repos[i], oid) == 0)
			return blob;

	return NULL;
}

/**
 * \brief Add directory to the list of Git repositories to search in.
 *
 * The repository will be opened until GitLoader object is destroyed.
 */
void GitLoader::addRepository(const char *git_dir)
{
	git_repository *repo;
	assert(git_repository_open(&repo, git_dir) == 0);

	m_repos.push_back(repo);
}

