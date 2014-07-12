#ifndef COMMIT_H
#define COMMIT_H

#include <gtpo/gitoid.h>

#include <git2/types.h>
#include <git2/oid.h>

#include <vector>
#include <string>

class CommitFileChange;

class Commit
{
public:
    Commit(git_commit* commit);
    ~Commit();

    const GitOid& oid() const;

    // TODO: rename to "date"
    // TODO: git_time_t -> FileDateTime
    git_time_t time() const;

    void addChange(CommitFileChange* change);

    int nChanges() const;
    const CommitFileChange *change(int index) const;

    /**
     * Returns the change (if any) to the file specified by "#{path}/#{name}"
     * in this commit.
     */
    const CommitFileChange* findChange(
        const std::string& name, const std::string& path) const;

    std::vector<const CommitFileChange*> findChangesByBasename(
        const std::string& basename) const;

    /**
     * Returns the change to the file specified by "#{path}/#{name}"
     * in this commit if that file was _removed_ here.
     */
    const git_oid* findRemovalOid(
        const std::string& name, const std::string& path) const;

    /**
     * Returns the change to the file specified by "#{path}/#{name}"
     * in this commit if that file was _added_ or _modified_ here.
     */
    const git_oid* findUpdateOid(
        const std::string& name, const std::string& path) const;

    std::vector<const git_oid*> findUpdateOidsByBasename(
        const std::string& basename) const;

public:
    GitOid m_oid;
    git_time_t m_time;

    std::vector<CommitFileChange*> m_changes;
};

#endif // COMMIT_H
