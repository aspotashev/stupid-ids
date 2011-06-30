#ifndef FILEDATETIME_H
#define FILEDATETIME_H

#include <time.h>
#include <string>

class FileDateTime
{
public:
	FileDateTime();

	/**
	 * @brief Copying constructor.
	 *
	 * @param o FileDateTime to copy from.
	 **/
	FileDateTime(const FileDateTime &o);

	std::string dateString() const;
	void clear();
	bool isNull() const;
	void fromString(const char *str);
	void setCurrentDateTime();
	bool operator<(const FileDateTime &o) const;
	bool operator>(const FileDateTime &o) const;

private:
	bool m_init;
	time_t m_date; // date/time in UTC
	int m_timezone; // only for displaying, does not affect actual date/time in UTC
};

#endif // FILEDATETIME_H

