
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "config.h"

StupidsConfig *StupidsConfig::s_instance = NULL;

StupidsConfig::StupidsConfig()
{
}

StupidsConfig::~StupidsConfig()
{
}

std::string strip_string(std::string str)
{
	int len = (int)str.size();

	int begin = 0;
	while (begin < len && isspace(str[begin]))
		begin ++;

	int end = len - 1;
	while (end >= 0 && isspace(str[end]))
		end --;

	if (begin > end) // no value, only spaces
		return std::string();

	return str.substr(begin, end - begin + 1);
}

void StupidsConfig::loadLine(char *buffer)
{
	if (buffer[0] == '#') // comment
		return;

	// remove possible newline character at the end
	while (buffer[strlen(buffer) - 1] == '\r' || buffer[strlen(buffer) - 1] == '\n')
		buffer[strlen(buffer) - 1] = '\0';

	char *eq_sign = strchr(buffer, '=');
	*eq_sign = '\0';
	std::string key = strip_string(std::string(buffer));
	std::string value = strip_string(std::string(eq_sign + 1));

	m_config[key] = value;
}

void StupidsConfig::loadConfig(const char *filename)
{
	char buffer[2000];

	FILE *f = fopen(filename, "r");
	assert(f);
	while (fgets(buffer, 2000, f))
		loadLine(buffer);
	fclose(f);
}

// static
StupidsConfig &StupidsConfig::defaultInstance()
{
	if (!s_instance)
	{
		s_instance = new StupidsConfig();
		s_instance->loadConfig(expand_path("~/.stupids.conf").c_str());
	}

	return *s_instance;
}

std::string StupidsConfig::operator()(const std::string &key)
{
	return m_config[key];
}

std::string expand_path(std::string path)
{
	if (path == std::string("~"))
		return std::string(getenv("HOME"));
	else if (path.size() >= 2 && path.substr(0, 2) == std::string("~/"))
		return std::string(getenv("HOME")) + path.substr(1);
	else
		return path;
}

