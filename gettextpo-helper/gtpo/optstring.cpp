#include "optstring.h"

OptString::OptString(const char* s)
    : m_isNull(false)
    , m_string()
{
    if (s)
        m_string = std::string(s);
    else
        m_isNull = true;
}

OptString::OptString(std::nullptr_t)
    : m_isNull(true)
    , m_string()
{
}

OptString::OptString(const std::string& s)
    : m_isNull(false)
    , m_string(s)
{
}

bool OptString::isNull() const
{
    return m_isNull;
}

bool OptString::empty() const
{
    return !isNull() && m_string.empty();
}

OptString::operator std::string() const
{
    if (m_isNull)
        throw NullStringException();
    else
        return m_string;
}

bool OptString::operator==(const OptString& o) const
{
    return (isNull() && o.isNull()) ||
        (!isNull() && !o.isNull() && m_string == o.m_string);
}

bool OptString::operator<(const OptString& o) const
{
    if (o.isNull()) // nothing is smaller than null
        return false;
    else if (isNull()) // null is smaller than non-null
        return true;
    else
        return *this < o;
}

const char* OptString::c_str() const
{
    return static_cast<std::string>(*this).c_str();
}

void OptString::setNull()
{
    m_isNull = true;
    std::string().swap(m_string);
}

std::ostream& operator<<(std::ostream& out, const OptString& s)
{
    return (out << (s.isNull() ? "(null)" : static_cast<std::string>(s)));
}

OptString::NullStringException::NullStringException()
    : std::exception()
{
}
