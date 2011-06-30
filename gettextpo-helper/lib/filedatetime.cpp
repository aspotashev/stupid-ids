#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include <gettextpo-helper/filedatetime.h>
#include <gettextpo-helper/gettextpo-helper.h>

FileDateTime::FileDateTime()
	: m_init(false)
{
}

std::string FileDateTime::dateString() const
{
	if (isNull())
		return std::string();

	char str[100];
	time_t display_time = m_date + 3600 * m_timezone;
	struct tm *tm = gmtime(&display_time);

	strftime(str, 99, "%Y-%m-%d %H:%M", tm);

	// append timezone
	sprintf(str + strlen(str), "%c%02d00",
		m_timezone >= 0 ? '+' : '-',
		abs(m_timezone));

	return std::string(str);
}

void FileDateTime::clear()
{
	m_init = false;
}

bool FileDateTime::isNull() const
{
	return !m_init;
}

void FileDateTime::fromString (const char* str)
{
	m_init = str && strlen(str) > 0;

	if (m_init)
	{
		m_timezone = 0;

		struct tm tm;
		tm.tm_sec = 0;
		tm.tm_wday = 0;
		tm.tm_yday = 0;
		tm.tm_isdst = 0;

		// Read date/time from string
		sscanf(str, "%d-%d-%d %d:%d%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &m_timezone);
		tm.tm_year -= 1900;
		tm.tm_mon --;

		assert(m_timezone % 100 == 0);
		m_timezone /= 100;

		// Calculate UNIX timestamp
		const char *old_tz = getenv("TZ");
		if (old_tz)
			old_tz = xstrdup(old_tz);

		assert(setenv("TZ", "UTC", 1) == 0); // make mktime() return time in UTC
		m_date = mktime(&tm) - 3600 * m_timezone; // date/time in UTC

		// Restore "TZ" environment variable
		if (old_tz)
		{
			assert(setenv("TZ", old_tz, 1) == 0);
			delete [] old_tz;
		}
		else
		{
			assert(unsetenv("TZ") == 0);
		}

//		printf("m_date = %Ld, date_str = %s, m_timezone = %d\n", (long long int)m_date, date_str, m_timezone);
//		std::cout << "dateString: " << dateString() << std::endl;
	}
}

void FileDateTime::setCurrentDateTime()
{
	m_init = true;
	m_date = time(NULL);


	struct tm *tmtime = localtime(&m_date);
	int offset = tmtime->tm_gmtoff;

	assert(offset % 3600 == 0);
	m_timezone = offset / 3600;
}
