
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "filedbfirstids.h"

FiledbFirstIds::FiledbFirstIds(const char *filename, const char *filename_next_id)
{
	FILE *f = fopen(filename, "r");
	assert(f);

	std::vector<GitOid> oids;
	std::vector<int> first_ids;

	char line[60];
	while (fgets(line, 60, f))
	{
		// tp_hash(40) + space(1) + first_id(1)
		assert(strlen(line) >= 42);
		// space between tp_hash and first_id
		assert(line[40] == ' ');

		line[40] = '\0'; // truncate at the end of tp_hash
		oids.push_back(GitOid(line));
		first_ids.push_back(atoi(line + 41));
	}
	fclose(f);

	// Read next_id.txt
	f = fopen(filename_next_id, "r");
	assert(f);

	assert(fgets(line, 60, f));
	int next_id = atoi(line);

	fclose(f);


	first_ids.push_back(next_id);

	for (size_t i = 0; i < oids.size(); i ++)
		m_firstIds[oids[i]] = std::make_pair<int, int>(
			first_ids[i], first_ids[i + 1] - first_ids[i]);
}

FiledbFirstIds::~FiledbFirstIds()
{
}

std::pair<int, int> FiledbFirstIds::getFirstId(const GitOid &tp_hash)
{
	std::map<GitOid, std::pair<int, int> >::const_iterator iter;
	iter = m_firstIds.find(tp_hash);

	if (iter != m_firstIds.end())
		return iter->second;
	else
		return std::make_pair<int, int>(0, 0); // tp_hash not found
}
