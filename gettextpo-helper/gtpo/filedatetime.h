#ifndef FILEDATETIME_H
#define FILEDATETIME_H

#include <git2.h>

#include <string>
#include <ctime>

class FileDateTime
{
public:
    FileDateTime();

    /**
     * @brief Copying constructor.
     *
     * @param o FileDateTime to copy from.
     **/
    FileDateTime(const FileDateTime& o);

    FileDateTime(git_time_t date);

    std::string dateString() const;
    void clear();
    bool isNull() const;
    void fromString(const std::string& str);
    void setCurrentDateTime();
    bool operator<(const FileDateTime& o) const;
    bool operator>(const FileDateTime& o) const;

    void addSeconds(int seconds);
    FileDateTime plusSeconds(int seconds) const;
    FileDateTime plusHours(int hours) const;
    FileDateTime plusDays(int days) const;

    operator git_time_t() const;

    bool isBetween(const FileDateTime &a, const FileDateTime &b) const;

private:
    bool m_init;
    time_t m_date; // date/time in UTC
    int m_timezone; // only for displaying, does not affect actual date/time in UTC
};

#endif // FILEDATETIME_H
