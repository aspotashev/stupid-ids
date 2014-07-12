#ifndef OPTSTRING_H
#define OPTSTRING_H

#include <exception>
#include <string>

/**
 * @brief Optional string - std::string + a null state
 *
 */
class OptString
{
public:
    class NullStringException;

    explicit OptString(const char* s);
    explicit OptString(std::nullptr_t);
    OptString(const std::string& s);

    bool isNull() const;
    bool empty() const;

    operator std::string() const;

    bool operator==(const OptString& o) const;
    bool operator<(const OptString& o) const;
    const char* c_str() const;

    void setNull();

private:
    bool m_isNull;
    std::string m_string;
};

std::ostream& operator<<(std::ostream &out, const OptString& s);

class OptString::NullStringException : public std::exception
{
public:
    NullStringException();
};

#endif // OPTSTRING_H
