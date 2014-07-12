#include "gitoid.h"

#include <stdexcept>

GitOid::GitOid()
{
    const unsigned char zero_oid_raw[GIT_OID_RAWSZ] = {0};
    git_oid_fromraw(&m_oid, zero_oid_raw);
}

GitOid::GitOid(const git_oid *oid)
{
    setOid(oid);
}

GitOid::GitOid(const char *oid_str)
{
    setOidStr(oid_str);
}

GitOid::~GitOid()
{
}

void GitOid::setOid(const git_oid *oid)
{
    git_oid_cpy(&m_oid, oid);
}

void GitOid::setOidStr(const char *oid_str)
{
    if (git_oid_fromstr(&m_oid, oid_str) != 0)
        throw std::runtime_error("git_oid_fromstr() failed for string: " + std::string(oid_str));
}

void GitOid::setOidRaw(const unsigned char *oid_raw)
{
    git_oid_fromraw(&m_oid, oid_raw);
}

bool GitOid::operator<(const GitOid &o) const
{
    return git_oid_cmp(&m_oid, &o.m_oid) < 0;
}

bool GitOid::operator==(const GitOid &o) const
{
    return git_oid_cmp(&m_oid, &o.m_oid) == 0;
}

bool GitOid::operator!=(const GitOid &o) const
{
    return git_oid_cmp(&m_oid, &o.m_oid) != 0;
}

const git_oid *GitOid::oid() const
{
    return &m_oid;
}

std::string GitOid::toString() const
{
    char oid_str[GIT_OID_HEXSZ + 1];
    oid_str[GIT_OID_HEXSZ] = '\0';
    git_oid_fmt(oid_str, &m_oid);

    return std::string(oid_str);
}

/**
 * \static
 */
GitOid GitOid::zero()
{
    return GitOid();
}
