#include "repository.h"
#include "commit.h"
#include "commitfilechange.h"

#include <algorithm>
#include <stdexcept>

#include <cstring>
#include <cassert>

Repository::Repository(const std::string& git_dir)
    : m_gitDir(git_dir)
    , m_libgitRepo(nullptr)
    , m_oidMaster(nullptr)
    , m_commitsInit(false)
{
}

Repository::~Repository()
{
    libgitClose();

    for (size_t i = 0; i < m_commits.size(); i ++)
        delete m_commits[i];
}

/**
 * \static
 */
git_tree* Repository::git_tree_entry_subtree(
    git_repository* repo, const git_tree_entry* entry)
{
    const git_oid* subtree_oid = git_tree_entry_id(entry);
    git_tree* subtree;
    assert(git_tree_lookup(&subtree, repo, subtree_oid) == 0);

    return subtree;
}

git_tree* Repository::git_tree_entry_subtree(const git_tree_entry* entry)
{
    assert(m_libgitRepo);

    return git_tree_entry_subtree(m_libgitRepo, entry);
}

std::string concat_path(const std::string& path, const std::string& name)
{
    // TBD: rewrite using some cross-platform library
    return path + "/" + name;
}

bool ends_with(const std::string& str, const std::string& ending)
{
    // TBD: rewrite using some cross-platform library
    size_t len = str.size();
    size_t ending_len = ending.size();
    return len >= ending_len && str.substr(len - ending_len) == ending;
}

bool is_dot_or_dotdot(const std::string& str)
{
    return str == "." || str == "..";
}

#define REPO_MODE_DIR 040000

int git_tree_entry_namecmp(const git_tree_entry *entry1, const git_tree_entry *entry2)
{
    // TODO: optimize this: avoid copying
    std::string name1(git_tree_entry_name(entry1));
    std::string name2(git_tree_entry_name(entry2));
    if (git_tree_entry_filemode(entry1) & REPO_MODE_DIR)
        name1 += "/";
    if (git_tree_entry_filemode(entry2) & REPO_MODE_DIR)
        name2 += "/";

    return strcmp(name1.c_str(), name2.c_str());
}

void Repository::diffTree(
    git_tree* tree1, git_tree* tree2,
    const std::string& path,
    Commit* currentCommit)
{
    // This should have been checked earlier
    if (tree1 != NULL && tree2 != NULL &&
        git_oid_cmp(git_tree_id(tree1), git_tree_id(tree2)) == 0)
    {
//      printf("Empty commit!\n");
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
            std::string name(git_tree_entry_name(entry1));
            if (git_tree_entry_filemode(entry1) & REPO_MODE_DIR)
            {
                git_tree* subtree1 = git_tree_entry_subtree(entry1);
                std::string subtree_path = path + "/" + name;

                diffTree(subtree1, NULL, subtree_path, currentCommit);
                git_tree_free(subtree1);
            }
            else
            {
                CommitFileChange* change = new CommitFileChange(
                    git_tree_entry_id(entry1), NULL,
                    path, name,
                    CommitFileChange::DEL);
                currentCommit->addChange(change);
//              change->print();
            }

            next1 = true;
        }
        else if (idx1 >= count1 || cmp > 0) // entry2 goes first, i.e. the entry is being added in this commit
        {
            const char *name = git_tree_entry_name(entry2);
            if (git_tree_entry_filemode(entry2) & REPO_MODE_DIR)
            {
                git_tree *subtree2 = git_tree_entry_subtree(entry2);
                std::string subtree_path = path + "/" + name;

                diffTree(NULL, subtree2, subtree_path, currentCommit);
                git_tree_free(subtree2);
            }
            else
            {
                CommitFileChange *change = new CommitFileChange(
                    NULL, git_tree_entry_id(entry2), path, name, CommitFileChange::ADD);
                currentCommit->addChange(change);
//              change->print();
            }

            next2 = true;
        }
        else // both entries exist and have the same names, i.e. the entry is being modified in this commit
        {
            if (git_oid_cmp(git_tree_entry_id(entry1), git_tree_entry_id(entry2)) == 0)
            {
//              printf("Entry unchanged.\n");
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
                    std::string subtree_path = path + "/" + name;

                    diffTree(subtree1, subtree2, subtree_path, currentCommit);
                    git_tree_free(subtree1);
                    git_tree_free(subtree2);
                }
                else // blob
                {
                    CommitFileChange *change = new CommitFileChange(
                        git_tree_entry_id(entry1), git_tree_entry_id(entry2), path, name, CommitFileChange::MOD);
                    currentCommit->addChange(change);
//                  change->print();
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
//      char oid_string[GIT_OID_HEXSZ + 1];
//      git_oid_to_string(oid_string, GIT_OID_HEXSZ + 1, git_commit_id(commit));
//      printf("Current commit's oid: %s\n", oid_string);

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
    std::reverse(m_commits.begin(), m_commits.end());

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

const git_oid* Repository::findLastRemovalOid(
    int from_commit,
    const std::string& name, const std::string& path) const
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

const git_oid* Repository::findNextUpdateOid(
    int from_commit,
    const std::string& name, const std::string& path) const
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

const git_oid* Repository::findLastUpdateOid(
    int from_commit,
    const std::string& name, const std::string& path) const
{
    assert(from_commit >= 0 && from_commit < nCommits());

    // Forward search in all commits newer than from_commit
    for (int i = from_commit; i >= 0; --i)
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
const git_oid *Repository::findFileOidByTime(
    git_time_t time,
    const std::string& name, const std::string& path) const
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
        int ret = git_repository_open(&m_libgitRepo, m_gitDir.c_str());
        if (ret != 0)
            throw std::runtime_error("Failed to open Git repository at " + std::string(m_gitDir));
    }

    return m_libgitRepo;
}

std::string Repository::gitDir() const
{
    return m_gitDir;
}

git_blob* Repository::blobLookup(const GitOid& oid)
{
    git_blob* blob;
    if (git_blob_lookup(&blob, libgitRepo(), oid.oid()) == 0)
        return blob;
    else
        return NULL;
}

int Repository::commitIndexByOid(const GitOid& oid) const
{
    for (int i = 0; i < nCommits(); i ++)
        if (oid == commit(i)->oid())
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
//      printf("(((%s)))\n", git_tree_entry_name(*iter));
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
