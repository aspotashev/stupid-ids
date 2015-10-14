#ifndef GITOID_H
#define GITOID_H

#include <git2/oid.h>

#include <string>

class GitOid
{
public:
    GitOid();
    GitOid(const git_oid* oid);
    GitOid(const char* oid_str);
    ~GitOid();

    void setOid(const git_oid* oid);
    void setOidStr(const char* oid_str);
    void setOidRaw(const unsigned char* oid_raw);

    bool operator<(const GitOid& o) const;
    bool operator==(const GitOid& o) const;
    bool operator!=(const GitOid& o) const;

    const git_oid* oid() const;

    std::string toString() const;

    bool isNull() const;

    static GitOid zero();

private:
    git_oid m_oid;
};

#endif // GITOID_H
