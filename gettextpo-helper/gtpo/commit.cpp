#include "commit.h"
#include "commitfilechange.h"

#include <git2/oid.h>
#include <git2/commit.h>

#include <cstring>
#include <cassert>

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

void Commit::addChange(CommitFileChange* change)
{
    m_changes.push_back(change);
}

int Commit::nChanges() const
{
    return (int)m_changes.size();
}

const CommitFileChange* Commit::change(int index) const
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

/**
 * Writes "#{path}/#{name}" into \a dest.
 */
std::string build_pathname(const std::string& path, const std::string& name)
{
    // TBD: rewrite using some cross-platform library
    return path + "/" + name;
}

const CommitFileChange* Commit::findChange(const std::string& name, const std::string& path) const
{
    // Using binary search (the items of m_changes are sorted by path+'/'+name)
    int left = 0;
    int right = nChanges() - 1;

    std::string pattern_fullname = build_pathname(path, name);
    while (left <= right)
    {
        int mid = (left + right) / 2;
        const CommitFileChange *change = this->change(mid);
        std::string fullname = build_pathname(change->path(), change->name());

        int cmp = strcmp(pattern_fullname.c_str(), fullname.c_str()); // rewrite away from C
        if (cmp > 0)
            left = mid + 1;
        else if (cmp < 0)
            right = mid - 1;
        else
            return change;
    }

    return NULL;
}

std::vector<const CommitFileChange*> Commit::findChangesByBasename(
    const std::string& basename) const
{
    std::vector<const CommitFileChange*> res;
    for (int i = 0; i < nChanges(); i ++)
        if (basename == change(i)->name())
            res.push_back(change(i));

    return res;
}

const git_oid* Commit::findRemovalOid(
    const std::string& name, const std::string& path) const
{
    const CommitFileChange *change = findChange(name, path);
    if (change && change->type() == CommitFileChange::DEL)
        return change->oid1();
    else
        return NULL;
}

// Finds addition or modification
const git_oid* Commit::findUpdateOid(
    const std::string& name, const std::string& path) const
{
    const CommitFileChange* change = findChange(name, path);
    if (change &&
        (change->type() == CommitFileChange::ADD ||
        change->type() == CommitFileChange::MOD))
        return change->oid2();
    else
        return NULL;
}

std::vector<const git_oid*> Commit::findUpdateOidsByBasename(
    const std::string& basename) const
{
    std::vector<const CommitFileChange*> changes = findChangesByBasename(basename);

    std::vector<const git_oid*> res;
    for (size_t i = 0; i < changes.size(); i ++)
        if (changes[i]->type() == CommitFileChange::ADD || changes[i]->type() == CommitFileChange::MOD)
            res.push_back(changes[i]->oid2());

    return res;
}
