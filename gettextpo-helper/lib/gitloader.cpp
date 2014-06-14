
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <set>
#include <algorithm>

#include <git2.h>

#include <gettextpo-helper/gitloader.h>
#include <gettextpo-helper/gettextpo-helper.h>
#include <gettextpo-helper/translationcontent.h>
#include <gettextpo-helper/oidmapcache.h>
#include <gettextpo-helper/stupids-client.h>

#define REPO_MODE_DIR 040000

int git_tree_entry_namecmp(const git_tree_entry *entry1, const git_tree_entry *entry2)
{
	// TODO: optimize this
	static char name1[5000];
	static char name2[5000];
	strcpy(name1, git_tree_entry_name(entry1));
	strcpy(name2, git_tree_entry_name(entry2));
	if (git_tree_entry_filemode(entry1) & REPO_MODE_DIR)
		strcat(name1, "/");
	if (git_tree_entry_filemode(entry2) & REPO_MODE_DIR)
		strcat(name2, "/");

	return strcmp(name1, name2);
//	return strcmp(git_tree_entry_name(entry1), git_tree_entry_name(entry2));
}

//----------------------------------------

Commit::Commit(git_commit *commit)
{
	git_oid_cpy(&m_oid, git_commit_id(commit));

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

const git_oid *Commit::oid() const
{
	return &m_oid;
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

/**
 * Writes "#{path}/#{name}" into \a dest.
 */
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

std::vector<const CommitFileChange *> Commit::findChangesByBasename(const char *basename) const
{
    std::vector<const CommitFileChange *> res;
    for (int i = 0; i < nChanges(); i ++)
        if (!strcmp(basename, change(i)->name()))
            res.push_back(change(i));

    return res;
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

std::vector<const git_oid *> Commit::findUpdateOidsByBasename(const char *basename) const
{
    std::vector<const CommitFileChange *> changes = findChangesByBasename(basename);

    std::vector<const git_oid *> res;
    for (size_t i = 0; i < changes.size(); i ++)
        if (changes[i]->type() == CommitFileChange::ADD || changes[i]->type() == CommitFileChange::MOD)
            res.push_back(changes[i]->oid2());

    return res;
}

//---------------------------------------

Repository::Repository(const char *git_dir)
{
	m_gitDir = xstrdup(git_dir);

	m_commitsInit = false;
	m_libgitRepo = NULL;
}

Repository::~Repository()
{
	libgitClose();

	delete [] m_gitDir;

	for (size_t i = 0; i < m_commits.size(); i ++)
		delete m_commits[i];
}

/**
 * \static
 */
git_tree *Repository::git_tree_entry_subtree(git_repository *repo, const git_tree_entry *entry)
{
	const git_oid *subtree_oid = git_tree_entry_id(entry);
	git_tree *subtree;
	assert(git_tree_lookup(&subtree, repo, subtree_oid) == 0);

	return subtree;
}

git_tree *Repository::git_tree_entry_subtree(const git_tree_entry *entry)
{
	assert(m_libgitRepo);

	return git_tree_entry_subtree(m_libgitRepo, entry);
}

char *concat_path(const char *path, const char *name)
{
	char *res = new char [strlen(path) + strlen(name) + 2];
	strcpy(res, path);
	strcat(res, "/");
	strcat(res, name);

	return res;
}

bool ends_with(const char *str, const char *ending)
{
	size_t len = strlen(str);
	size_t ending_len = strlen(ending);
	return len >= ending_len && strcmp(str + len - ending_len, ending) == 0;
}

bool is_dot_or_dotdot(const char *str)
{
	return strcmp(str, ".") == 0 || strcmp(str, "..") == 0;
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
	// TODO: optimize this (according to Callgrind, diffTree() takes 79.76% of time)
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
			if (git_tree_entry_filemode(entry1) & REPO_MODE_DIR)
			{
				git_tree *subtree1 = git_tree_entry_subtree(entry1);
				char *subtree_path = concat_path(path, name);

				diffTree(subtree1, NULL, subtree_path, currentCommit);
				delete [] subtree_path;
				git_tree_free(subtree1);
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
			if (git_tree_entry_filemode(entry2) & REPO_MODE_DIR)
			{
				git_tree *subtree2 = git_tree_entry_subtree(entry2);
				char *subtree_path = concat_path(path, name);

				diffTree(NULL, subtree2, subtree_path, currentCommit);
				delete [] subtree_path;
				git_tree_free(subtree2);
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
				unsigned int attr1 = git_tree_entry_filemode(entry1);
				unsigned int attr2 = git_tree_entry_filemode(entry2);
				if (attr1 != attr2 && !(attr1 == 0100755 && attr2 == 0100644))
				{
					printf("Git tree entry attributes are changing in this commit: time = %lld\n", (long long int)currentCommit->time());
					printf("    Old attributes: %o (octal)\n", attr1);
					printf("    New attributes: %o (octal)\n", attr2);
					assert(0);
				}

				const char *name = git_tree_entry_name(entry1);
				if (attr1 & REPO_MODE_DIR) // tree
				{
					git_tree *subtree1 = git_tree_entry_subtree(entry1);
					git_tree *subtree2 = git_tree_entry_subtree(entry2);
					char *subtree_path = concat_path(path, name);

					diffTree(subtree1, subtree2, subtree_path, currentCommit);
					delete [] subtree_path;
					git_tree_free(subtree1);
					git_tree_free(subtree2);
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
		git_tree_free(tree1);
	if (tree2)
		git_tree_free(tree2);
}

/**
 * Initializes m_oidMaster.
 */
void Repository::initOidMaster()
{
	libgitRepo();

	git_reference *ref_master;
	assert(git_reference_lookup(&ref_master, m_libgitRepo, "refs/heads/master") == 0);

	m_oidMaster = new git_oid;
	git_oid_cpy(m_oidMaster, git_reference_target(ref_master));
}

void Repository::readRepositoryCommits()
{
	if (m_commitsInit)
		return;

	libgitRepo();
	initOidMaster();

	// Read repository
	git_commit *commit;
	git_commit *parent;
	assert(git_commit_lookup(&commit, m_libgitRepo, m_oidMaster) == 0);

	while (commit != NULL)
	{
//		char oid_string[GIT_OID_HEXSZ + 1];
//		git_oid_to_string(oid_string, GIT_OID_HEXSZ + 1, git_commit_id(commit));
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

		git_commit_free(commit);
		commit = parent;
	}

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

int Repository::lastCommitByTime(git_time_t time) const
{
	if (nCommits() == 0 || time < commit(0)->time())
		return -1; // Nothing was commited before the given time

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

	assert(0); // This cannot happen. Could not find the commit, but it must be between 'l' and 'r'.
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

int Repository::nearestCommitByTime(git_time_t time) const
{
    int commit = lastCommitByTime(time);
    return commit == -1 ? 0 : commit;
}

std::vector<int> Repository::listCommitsBetweenDates(const FileDateTime &date_a, const FileDateTime &date_b)
{
    readRepositoryCommits();

    bool backward = date_b < date_a;

//    printf("date_a=%s, date_b=%s\n", date_a.dateString().c_str(), date_b.dateString().c_str());
    int less = nearestCommitByTime((git_time_t)date_a);
    int more = nearestCommitByTime((git_time_t)date_b);
    if (backward)
        std::swap(less, more);

    less = std::max(less - 5, 0);
    more = std::min(more + 5, nCommits() - 1);
    std::vector<int> res;
    for (int i = less; i <= more; i ++)
        if (FileDateTime(commit(i)->time()).isBetween(date_a, date_b))
            res.push_back(i);

    // Sort "res" by date ("backward" => accending or descending order)
    sort(res.begin(), res.end(), Repository::compare_commits_by_date(this, backward));

    return res;
}

const git_oid *Repository::findRelevantPot(const char *basename, const FileDateTime &date)
{
//    printf("Repository::findRelevantPot: filename=%s, date=%s\n", basename, date.dateString().c_str());

    // Between NOW-3hours and NOW+14days
    std::vector<int> commits = listCommitsBetweenDates(date.plusHours(-3), date.plusDays(14));
    // Before NOW-3hours
    std::vector<int> commits_prev = listCommitsBetweenDates(date.plusHours(-3).plusSeconds(-1), FileDateTime(0)); // TODO: replace "FileDateTime(0)" with "FileDateTime::MinDate" (needs to be implemented firstly)

    // Append "commits_prev" at the end of "commits".
    size_t commits_size = commits.size();
    commits.resize(commits.size() + commits_prev.size());
    std::copy(commits_prev.begin(), commits_prev.end(), commits.begin() + commits_size);

    const git_oid *best_po_oid = NULL;
    for (size_t i = 0; i < commits.size(); i ++)
    {
        std::vector<const git_oid *> po_oids = commit(commits[i])->findUpdateOidsByBasename(basename);
        assert(po_oids.size() <= 1); // too strange to have 2 files with the same name changed in one commit

        if (!po_oids.empty())
        {
            best_po_oid = po_oids[0];
            break;
        }
    }

    return best_po_oid;
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

void Repository::libgitClose()
{
	if (m_libgitRepo)
	{
		git_repository_free(m_libgitRepo);
		m_libgitRepo = NULL;
	}
}

git_repository *Repository::libgitRepo()
{
	if (!m_libgitRepo)
	{
		assert(git_repository_open(&m_libgitRepo, m_gitDir) == 0);
	}

	return m_libgitRepo;
}

const char *Repository::gitDir() const
{
	return m_gitDir;
}

git_blob *Repository::blobLookup(const git_oid *oid)
{
	git_blob *blob;
	if (git_blob_lookup(&blob, libgitRepo(), oid) == 0)
		return blob;
	else
		return NULL;
}

int Repository::commitIndexByOid(const git_oid *oid) const
{
	for (int i = 0; i < nCommits(); i ++)
		if (git_oid_cmp(oid, commit(i)->oid()) == 0)
			return i;

	return -1; // commit not found
}

/**
 * \brief Returns the list of OIDs of all .po translation files currently
 * present in "master" branch of this repository.
 */
std::vector<GitOid> Repository::getCurrentOids()
{
	std::vector<GitOid> res;

	libgitRepo();

	git_commit *commit;
	initOidMaster();
	assert(git_commit_lookup(&commit, m_libgitRepo, m_oidMaster) == 0); // TODO: make this a function

	blob_iterator iter;
	iter.initBegin(this, commit); // TODO: Repository::blob_begin(git_commit *)
	for (; !iter.isEnd(); iter ++)
	{
		const char *name = git_tree_entry_name(*iter);
		if (strcmp(name + strlen(name) - 3, ".po") != 0)
			continue;

		res.push_back(git_tree_entry_id(*iter));
//		printf("(((%s)))\n", git_tree_entry_name(*iter));
	}

	return res;
}

bool Repository::compareCommitsByDate(int a, int b, bool descending) const
{
    assert(a >= 0 && a < nCommits() && b >= 0 && b < nCommits());

    if (descending)
        return commit(a)->time() > commit(b)->time();
    else
        return commit(a)->time() < commit(b)->time();
}

//---------------------------------------------------------

// "Enters" the directory
void Repository::blob_iterator::enterDir(git_tree *tree)
{
	tree_info ti;
	ti.tree = tree;
	ti.count = git_tree_entrycount(tree);
	ti.idx = 0;

	if (ti.count > 0)
		m_stack.push(ti);
	else // empty tree
		git_tree_free(tree);
}

/**
 * Does not take ownership of neither @p repo nor @p commit.
 */
void Repository::blob_iterator::initBegin(Repository *repo, git_commit *commit)
{
	assert(repo);
	assert(commit);

	// TODO: guarantee that iterator is deleted
	// when "Repository" object gets deleted,
	// otherwise we get a pointer to a destroyed "git_repository" here
	// after "Repository" object is deleted.
	m_repo = repo->libgitRepo();

	// root tree
	git_tree *tree;
	tree = NULL;
	assert(git_commit_tree(&tree, commit) == 0);

	enterDir(tree);
	walkBlob();
}

void Repository::blob_iterator::walkBlob()
{
	if (isEnd())
		return;

	while (!isEnd())
	{
		const git_tree_entry *entry = NULL;
		tree_info &ti = m_stack.top();

		// We assume that tree entries are sorted by name
		assert(ti.idx >= ti.count - 1 || git_tree_entry_namecmp(
			git_tree_entry_byindex(ti.tree, ti.idx),
			git_tree_entry_byindex(ti.tree, ti.idx + 1)) < 0);

		if (ti.idx == ti.count) // directory ends
		{
			// Exit directory
			git_tree_free(ti.tree);
			m_stack.pop();

			// Go to the next entry in the parent directory
			if (!isEnd())
				m_stack.top().idx ++;
		}
		else if (git_tree_entry_filemode(entry = git_tree_entry_byindex(ti.tree, ti.idx)) & REPO_MODE_DIR) // directory
		{
			enterDir(Repository::git_tree_entry_subtree(m_repo, entry));
		}
		else
		{
			break; // blob found
		}
	}
}

void Repository::blob_iterator::increment()
{
	assert(!isEnd());

	m_stack.top().idx ++;
	walkBlob();
}

const git_tree_entry *Repository::blob_iterator::operator*() const
{
	assert(!isEnd());

	const tree_info &ti = m_stack.top();
	assert(ti.idx >= 0 && ti.idx < ti.count);

	return git_tree_entry_byindex(ti.tree, ti.idx);
}

Repository::blob_iterator &Repository::blob_iterator::operator++()
{
	increment();
	return *this;
}

// postfix "++"
Repository::blob_iterator Repository::blob_iterator::operator++(int)
{
	blob_iterator tmp = *this;
	++(*this);
	return tmp;
}

bool Repository::blob_iterator::isEnd() const
{
	return m_stack.size() == 0;
}

//---------------------------------------------------------

Repository::compare_commits_by_date::compare_commits_by_date(const Repository *repo, bool descending)
    : m_repo(repo)
    , m_descending(descending)
{
}

bool Repository::compare_commits_by_date::operator()(int a, int b) const
{
    return m_repo->compareCommitsByDate(a, b, m_descending);
}

//---------------------------------------------------------

GitLoader::GitLoader()
{
}

GitLoader::~GitLoader()
{
	for (size_t i = 0; i < m_repos.size(); i ++)
		delete m_repos[i];
}

/**
 * \brief Search blob by OID in all repositories added using addRepository().
 *
 * \param oid Git object ID of the blob.
 *
 * \returns Blob or NULL, if it was not found in any of the repositories.
 * It is necessary to call the function "git_blob_free" when you
 * stop using a blob. Failure to do so will cause a memory leak.
 */
git_blob *GitLoader::blobLookup(const git_oid *oid)
{
	git_blob *blob;
	for (size_t i = 0; i < m_repos.size(); i ++)
		if ((blob = m_repos[i]->blobLookup(oid)))
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
	m_repos.push_back(new Repository(git_dir));
}

const git_oid *GitLoader::findOldestByTphash_oid(const git_oid *tp_hash)
{
	// Cache results of this function (TODO: may be even create a stupids-server command for this function?)
	const git_oid *cached_oid = OidMapCacheManager::instance("lang_ru_oldest_oid").getValue(tp_hash);
	if (cached_oid)
		return cached_oid;

	// TODO: ch_iterator (iterator for walking through all CommitFileChanges)
	// TODO: better: "ch_iterator_time" -- walking through all CommitFileChanges sorted by time (BONUS: starting at the given time)
	for (size_t i = 0; i < m_repos.size(); i ++)
	{
		Repository *repo = m_repos[i];

		repo->readRepositoryCommits();
		// TODO: check only .po files committed after POT-Creation-Date?
		for (int j = 0; j < repo->nCommits(); j ++)
		{
			const Commit *commit = repo->commit(j);
			for (int k = 0; k < commit->nChanges(); k ++)
			{
				const CommitFileChange *change = commit->change(k);
				const char *name = change->name();
				size_t len = strlen(name);
				if (strcmp(name + len - 3, ".po") != 0)
					continue;

				const git_oid *oid = change->tryOid2();
				if (!oid)
					continue;

				TranslationContent *content = new TranslationContent(this, oid);
				const git_oid *current_tp_hash = content->calculateTpHash();
				if (current_tp_hash && git_oid_cmp(tp_hash, current_tp_hash) == 0)
				{
					delete content;
					OidMapCacheManager::instance("lang_ru_oldest_oid").addPair(tp_hash, oid);
					return oid; // TODO: choose the oldest TranslationContent from _all_ repositories
				}
				else
				{
					delete content;
				}
			}
		}
	}

	return NULL;
}

TranslationContent *GitLoader::findOldestByTphash(const git_oid *tp_hash)
{
	const git_oid *oid = findOldestByTphash_oid(tp_hash);
	return oid ? new TranslationContent(this, oid) : NULL;
}

/**
 * \brief Returns an STL vector of IDs of all messages currently
 * present in "master" branch in any of the repositories.
 */
std::vector<int> GitLoader::getCurrentIdsVector()
{
	std::vector<GitOid> oids;
	for (size_t i = 0; i < m_repos.size(); i ++)
	{
		std::vector<GitOid> cur = m_repos[i]->getCurrentOids();
		for (size_t j = 0; j < cur.size(); j ++)
			oids.push_back(cur[j]);
	}

	std::vector<GitOid> tp_hashes;
	for (size_t i = 0; i < oids.size(); i ++)
	{
		TranslationContent *content = new TranslationContent(this, oids[i].oid());
		const git_oid *tp_hash = content->calculateTpHash();
		if (tp_hash)
			tp_hashes.push_back(GitOid(tp_hash));
		else
		{
			printf("Warning: unknown tp-hash\n");
			assert(0);
		}
	}

	std::vector<std::pair<int, int> > first_ids = stupidsClient.getFirstIdPairs(tp_hashes);

	std::vector<int> res;
	for (size_t i = 0; i < first_ids.size(); i ++)
	{
		int first_id = first_ids[i].first;
		int id_count = first_ids[i].second;

		if (first_id == 0)
			continue;
		assert(id_count >= 0);

		for (int id = first_id; id < first_id + id_count; id ++)
			res.push_back(id);
	}

	return res;
}

