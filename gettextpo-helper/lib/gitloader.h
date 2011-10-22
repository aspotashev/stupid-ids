
#ifndef GITLOADER_H
#define GITLOADER_H

#include <vector>
#include <stack>

#include <git2.h>

#include <gettextpo-helper/detectorbase.h>
#include <gettextpo-helper/filedatetime.h>

class CommitFileChange;

class Commit
{
public:
	Commit(git_commit *commit);
	~Commit();

	const git_oid *oid() const;

    // TODO: rename to "date"
    // TODO: git_time_t -> FileDateTime
	git_time_t time() const;

	void addChange(CommitFileChange *change);

	int nChanges() const;
	const CommitFileChange *change(int index) const;

    /**
     * Returns the change (if any) to the file specified by "#{path}/#{name}"
     * in this commit.
     */
	const CommitFileChange *findChange(const char *name, const char *path) const;

    std::vector<const CommitFileChange *> findChangesByBasename(const char *basename) const;

    /**
     * Returns the change to the file specified by "#{path}/#{name}"
     * in this commit if that file was _removed_ here.
     */
	const git_oid *findRemovalOid(const char *name, const char *path) const;

    /**
     * Returns the change to the file specified by "#{path}/#{name}"
     * in this commit if that file was _added_ or _modified_ here.
     */
	const git_oid *findUpdateOid(const char *name, const char *path) const;

    std::vector<const git_oid *> findUpdateOidsByBasename(const char *basename) const;

public:
	git_oid m_oid;
	git_time_t m_time;

	std::vector<CommitFileChange *> m_changes;
};

//----------------------------------------

class CommitFileChange
{
public:
	CommitFileChange(
		const git_oid *oid1, const git_oid *oid2,
		const char *path, const char *name, int type);
	~CommitFileChange();

	void print() const;

	const git_oid *oid1() const;
	const git_oid *oid2() const;
	const char *path() const;
	const char *name() const;
	int type() const;

	const git_oid *tryOid1() const;
	const git_oid *tryOid2() const;

	enum
	{
		ADD = 1,
		DEL = 2,
		MOD = 3
	};

private:
	git_oid m_oid1;
	git_oid m_oid2;
	char *m_path;
	char *m_name;
	int m_type;
};

//----------------------------------------

class GitLoaderBase
{
public:
	virtual git_blob *blobLookup(const git_oid *oid) = 0;
};

class Repository : public GitLoaderBase
{
public:
	Repository(const char *git_dir);
	~Repository();

	void readRepositoryCommits();

	int nCommits() const;
	/// In the order from repository root (index = 0) to HEAD (index = nCommits - 1)
	const Commit *commit(int index) const;
	int commitIndexByOid(const git_oid *oid) const;

	const git_oid *findLastRemovalOid(int from_commit, const char *name, const char *path) const;
	const git_oid *findNextUpdateOid(int from_commit, const char *name, const char *path) const;
	const git_oid *findLastUpdateOid(int from_commit, const char *name, const char *path) const;

    /**
     * Returns the latest commit done as of the given time
     * (before the given time or exactly at the given time).
     */
	int lastCommitByTime(git_time_t time) const;

    int nearestCommitByTime(git_time_t time) const;

	const git_oid *findFileOidByTime(git_time_t time, const char *name, const char *path) const;

    /**
     * Finds the appropriate version of #{filename}.pot in the following order:
     *    1. Forward on the timeline up to 2 weeks after the given date
     *         (because sometimes "scripty" is shut down for some time)
     *    2. If nothing has been found between the given date and 2 weeks
     *         after it, use backward search (just find the "current"
     *         version of the .pot).
     *
     * \param filename File basename (without ".pot" or ".po" extension)
     * \param date Search around this date
     */
    const git_oid *findRelevantPot(const char *basename, const FileDateTime &date);

    std::vector<int> listCommitsBetweenDates(const FileDateTime &date_a, const FileDateTime &date_b);

	void dumpOids(std::vector<GitOid2Change> &dest) const;

	void libgitClose();
	git_repository *libgitRepo();
	virtual git_blob *blobLookup(const git_oid *oid);

	const char *gitDir() const;

	std::vector<GitOid> getCurrentOids();

    // Returns "true" if "a.date" is less (i.e. older) than "b.date" (if "descending" is false)
    bool compareCommitsByDate(int a, int b, bool descending) const;

private:
	static git_tree *git_tree_entry_subtree(git_repository *repo, const git_tree_entry *entry);
	git_tree *git_tree_entry_subtree(const git_tree_entry *entry);

	void initOidMaster();

	void diffTree(git_tree *tree1, git_tree *tree2, const char *path, Commit *currentCommit);
	void diffCommit(git_commit *commit1, git_commit *commit2);

	class blob_iterator;
    class compare_commits_by_date;

private:
	char *m_gitDir;
	git_repository *m_libgitRepo;
	git_oid *m_oidMaster;

	std::vector<Commit *> m_commits;
	bool m_commitsInit;
};

class Repository::blob_iterator
{
public:
	void initBegin(Repository *repo, git_commit *commit);

	const git_tree_entry *operator*() const;
	blob_iterator &operator++();
	blob_iterator operator++(int); // postfix "++"
	void increment();
	bool isEnd() const;

private:
	void enterDir(git_tree *tree);
	void walkBlob();


	struct tree_info
	{
		git_tree *tree;
		size_t count;
		size_t idx;
	};

	// TODO: stack is not a good solution when you need to walk from the bottom to the top
	// (for example, in order to construct the full path for the current entry)
	std::stack<tree_info> m_stack;
	git_repository *m_repo; // needed for git_tree_entry_subtree
};

class Repository::compare_commits_by_date
{
public:
    compare_commits_by_date(const Repository *repo, bool descending);

    // Returns "true" if "a" is less than "b".
    bool operator()(int a, int b) const;

private:
    const Repository *m_repo;
    bool m_descending;
};

//----------------------------------------

class TranslationContent;

/** Class for loading files by their Git OIDs from multiple Git repositories.
 */
class GitLoader : public GitLoaderBase
{
public:
	GitLoader();
	~GitLoader();

	virtual git_blob *blobLookup(const git_oid *oid);
	void addRepository(const char *git_dir);

	// Finds the oldest (in Git repositories' histories) .po file with the given tp_hash.
	TranslationContent *findOldestByTphash(const git_oid *tp_hash);

	std::vector<int> getCurrentIdsVector();

private:
	const git_oid *findOldestByTphash_oid(const git_oid *tp_hash);

private:
	std::vector<Repository *> m_repos;
};

char *concat_path(const char *path, const char *name);
bool ends_with(const char *str, const char *ending);
bool is_dot_or_dotdot(const char *str);


#endif // GITLOADER_H

