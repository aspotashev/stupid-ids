
#include <map>

#include <gettextpo-helper/detectorbase.h>

class FiledbFirstIds
{
public:
	FiledbFirstIds(const char *filename, const char *filename_next_id);
	~FiledbFirstIds();

	std::pair<int, int> getFirstId(const GitOid &tp_hash);

private:
	std::vector<std::pair<GitOid, std::pair<int, int> > > m_firstIds;
};

