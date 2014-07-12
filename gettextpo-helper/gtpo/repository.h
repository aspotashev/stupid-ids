#ifndef REPOSITORY_H
#define REPOSITORY_H

#include "gitloader.h"

class Commit;

class Repository : public GitLoaderBase
{
public:
    Repository(const std::string& git_dir);
    ~Repository();

    void readRepositoryCommits();

    int nCommits() const;
    /// In the order from repository root (index = 0) to HEAD (index = nCommits - 1)
    const Commit* commit(int index) const;
    int commitIndexByOid(const git_oid* oid) const;

    const git_oid* findLastRemovalOid(
        int from_commit,
        const std::string& name, const std::string& path) const;
    const git_oid* findNextUpdateOid(
        int from_commit,
        const std::string& name, const std::string& path) const;
    const git_oid* findLastUpdateOid(
        int from_commit,
        const std::string& name, const std::string& path) const;

    /**
     * Returns the latest commit done as of the given time
     * (before the given time or exactly at the given time).
     */
    int lastCommitByTime(git_time_t time) const;

    int nearestCommitByTime(git_time_t time) const;

    const git_oid* findFileOidByTime(
        git_time_t time,
        const std::string& name, const std::string& path) const;

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

    std::string gitDir() const;

    std::vector<GitOid> getCurrentOids();

    // Returns "true" if "a.date" is less (i.e. older) than "b.date" (if "descending" is false)
    bool compareCommitsByDate(int a, int b, bool descending) const;

private:
    static git_tree *git_tree_entry_subtree(git_repository *repo, const git_tree_entry *entry);
    git_tree *git_tree_entry_subtree(const git_tree_entry *entry);

    void initOidMaster();

    void diffTree(
        git_tree* tree1, git_tree* tree2,
        const std::string& path,
        Commit* currentCommit);
    void diffCommit(git_commit* commit1, git_commit* commit2);

    class blob_iterator;
    class compare_commits_by_date;

private:
    std::string m_gitDir;
    git_repository* m_libgitRepo;
    git_oid* m_oidMaster;

    bool m_commitsInit;
    std::vector<Commit*> m_commits;
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

#endif // REPOSITORY_H
