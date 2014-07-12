#include "commitfilechange.h"

#include <string>
#include <iostream>

#include <cassert>

int CommitFileChange::type() const
{
    return m_type;
}

CommitFileChange::CommitFileChange(
    const git_oid* oid1, const git_oid* oid2,
    const std::string& path, const std::string& name, int type)
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

    if (path[0] == '/')
        m_path = path.substr(1);
    else
        m_path = path;

    m_name = name;

    m_type = type;
}

CommitFileChange::~CommitFileChange()
{
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

std::string CommitFileChange::path() const
{
    return m_path;
}

std::string CommitFileChange::name() const
{
    return m_name;
}

void CommitFileChange::print() const
{
    switch (m_type)
    {
    case ADD:
        // a file was added
        std::cout << "A    (" << m_path << ") " << m_name << std::endl;
        break;
    case DEL:
        // a file was removed
        std::cout << "D    (" << m_path << ") " << m_name << std::endl;
        break;
    case MOD:
        // a file was modified
        std::cout << "M    (" << m_path << ") " << m_name << std::endl;
        break;
    default:
        assert(0);
    }
}
