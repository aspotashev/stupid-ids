
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "processorphans.h"
#include "xstrdup.h"

ProcessOrphansTxtEntry::ProcessOrphansTxtEntry(const char *cmd, const char *origin, const char *destination)
{
	if (!strcmp(cmd, "move"))
		m_type = MOVE;
	else if (!strcmp(cmd, "copy"))
		m_type = COPY;
	else if (!strcmp(cmd, "delete"))
		m_type = DELETE;
	else if (!strcmp(cmd, "merge"))
		m_type = MERGE;
	else
		assert(0); // Unknown command


	assert(origin);

	// If command is not "delete", destination must be specified.
	//
	// If command is "delete", destination may
	// contain garbage (for example, SVN revision number).
	assert(m_type == DELETE || destination != NULL);


	m_origin = xstrdup(origin);

	m_destination = NULL;
	if (m_type != DELETE)
		m_destination = xstrdup(destination);
}

ProcessOrphansTxtEntry::~ProcessOrphansTxtEntry()
{
	delete [] m_origin;

	if (m_type != DELETE)
		delete [] m_destination;
}

//-------------------------------------------------------

ProcessOrphansTxt::ProcessOrphansTxt(const char *filename)
{
	FILE *f = fopen(filename, "r");

	static char line[10000];
	while (1)
	{
		if (!fgets(line, 10000, f))
			break;

		if (line[0] == '\0' || line[0] == '\n' || line[0] == '\r') // empty
			continue;
		if (line[0] == '#') // comment
			continue;

		char *origin = strchr(line, ' ');
		if (!origin)
		{
			printf("Space not found in string: [%s]\n", line);
			assert(0);
		}
		*origin = '\0'; // cut off the command (in 'line')
		origin ++; // moving to the beginning of the origin path

		char *destination = strchr(origin, ' ');
		if (!destination) // command "delete" does not require destination
		{
			origin[strcspn(origin, " \n\r")] = '\0';
		}
		else
		{
			*destination = '\0'; // cut off the origin path (in 'origin')
			destination ++; // moving to the beginning of the destination path

			destination[strcspn(destination, " \n\r")] = '\0';
		}

//		printf("cmd = [%s]\norigin = [%s]\ndest = [%s]\n\n", line, origin, destination);
		m_entries.push_back(new ProcessOrphansTxtEntry(line, origin, destination));
	}
}

ProcessOrphansTxt::~ProcessOrphansTxt()
{
}

