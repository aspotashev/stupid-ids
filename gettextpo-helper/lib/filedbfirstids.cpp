
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <algorithm>
#include <stdexcept>

#include "filedbfirstids.h"

FiledbFirstIds::FiledbFirstIds(const char *filename, const char *filename_next_id)
{
	FILE *f = fopen(filename, "r");
        if (!f)
            throw std::runtime_error("Failed to open file " + std::string(filename));

	std::vector<GitOid> oids;
	std::vector<int> first_ids;

	char line[60];
	while (fgets(line, 60, f))
	{
		// tp_hash(40) + space(1) + first_id(1)
		if (strlen(line) < 42) {
                    throw std::runtime_error("Line is too short: " + std::string(line));
                }

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
		m_firstIds.push_back(std::pair<GitOid, std::pair<int, int> >(oids[i], std::pair<int, int>(
			first_ids[i], first_ids[i + 1] - first_ids[i])));
	sort(m_firstIds.begin(), m_firstIds.end());
}

FiledbFirstIds::~FiledbFirstIds()
{
}

std::pair<int, int> FiledbFirstIds::getFirstId(const GitOid &tp_hash)
{
	std::vector<std::pair<GitOid, std::pair<int, int> > >::iterator iter = lower_bound(
		m_firstIds.begin(), m_firstIds.end(),
		std::pair<GitOid, std::pair<int, int> >(tp_hash, std::pair<int, int>(0, -1)));

	if (iter != m_firstIds.end() && tp_hash == iter->first)
		return iter->second;
	else
		return std::make_pair<int, int>(0, -1); // tp_hash not found
}

