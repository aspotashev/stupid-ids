#ifndef FILEDATETIME_H
#define FILEDATETIME_H

#include <string>

class FileDateTime
{
public:
	FileDateTime();

	std::string dateString() const;
	void clear();
	bool isNull() const;
	void fromString(const char *str);
	void setCurrentDateTime();

private:
	bool m_init;
	time_t m_date; // date/time in UTC
	int m_timezone; // only for displaying, does not affect actual date/time in UTC
};

#endif // FILEDATETIME_H

