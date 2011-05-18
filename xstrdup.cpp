
#include <string.h>

// TODO: use common code with gettextpo-helper/lib/gettextpo-helper.cpp
char *xstrdup(const char *str)
{
	size_t len = strlen(str);
	char *dup = new char [len + 1];
	strcpy(dup, str);

	return dup;
}

