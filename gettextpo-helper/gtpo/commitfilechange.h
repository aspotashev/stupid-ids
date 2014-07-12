#ifndef COMMITFILECHANGE_H
#define COMMITFILECHANGE_H

#include <git2/oid.h>

#include <string>

class CommitFileChange
{
public:
    CommitFileChange(
        const git_oid* oid1, const git_oid* oid2,
        const std::string& path, const std::string& name, int type);
    ~CommitFileChange();

    void print() const;

    const git_oid* oid1() const;
    const git_oid* oid2() const;
    std::string path() const;
    std::string name() const;
    int type() const;

    const git_oid* tryOid1() const;
    const git_oid* tryOid2() const;

    enum
    {
        ADD = 1,
        DEL = 2,
        MOD = 3
    };

private:
    git_oid m_oid1;
    git_oid m_oid2;
    std::string m_path;
    std::string m_name;
    int m_type;
};

#endif // COMMITFILECHANGE_H
