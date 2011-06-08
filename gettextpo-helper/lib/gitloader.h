
#ifndef GITLOADER_H
#define GITLOADER_H

#include <vector>

#include <git2.h>

#include <gettextpo-helper/detectorbase.h>

class CommitFileChange;

class Commit
{
public:
	Commit(git_commit *commit);
	~Commit();

	git_time_t time() const;

	void addChange(CommitFileChange *change);

	int nChanges() const;
	const CommitFileChange *change(int index) const;

	const CommitFileChange *findChange(const char *name, const char *path) const;
	const git_oid *findRemovalOid(const char *name, const char *path) const;
	const git_oid *findUpdateOid(const char *name, const char *path) const;

public:
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

class Repository
{
public:
	Repository(const char *git_dir);
	~Repository();

	void readRepositoryCommits();

	int nCommits() const;
	const Commit *commit(int index) const;

	const git_oid *findLastRemovalOid(int from_commit, const char *name, const char *path) const;
	const git_oid *findNextUpdateOid(int from_commit, const char *name, const char *path) const;
	const git_oid *findLastUpdateOid(int from_commit, const char *name, const char *path) const;

	int lastCommitByTime(git_time_t time) const;
	const git_oid *findFileOidByTime(git_time_t time, const char *name, const char *path) const;

	void dumpOids(std::vector<GitOid2Change> &dest) const;

private:
	git_tree *git_tree_entry_subtree(const git_tree_entry *entry);

	void diffTree(git_tree *tree1, git_tree *tree2, const char *path);
	void diffCommit(git_commit *commit1, git_commit *commit2);

private:
	char *m_gitDir;
	git_repository *m_repo;

	const git_oid *m_oidMaster;
	Commit *m_currentCommit;

	std::vector<Commit *> m_commits;
	bool m_commitsInit;
};

//----------------------------------------

/** Class for loading files by their Git OIDs from multiple Git repositories.
 */
class GitLoader
{
public:
	GitLoader();
	~GitLoader();

	git_blob *blobLookup(const git_oid *oid);
	void addRepository(const char *git_dir);

private:
	std::vector<git_repository *> m_repos;
};

#endif // GITLOADER_H

