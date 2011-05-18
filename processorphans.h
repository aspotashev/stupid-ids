
#include <vector>

class ProcessOrphansTxtEntry
{
public:
	ProcessOrphansTxtEntry(const char *cmd, const char *origin, const char *destination);
	~ProcessOrphansTxtEntry();

private:
	int m_type;
	char *m_origin;
	char *m_destination;

	enum
	{
		MOVE = 1,
		COPY = 2,
		DELETE = 3,
		MERGE = 4
	};
};

//-------------------------------------------------------

class ProcessOrphansTxt
{
public:
	ProcessOrphansTxt(const char *filename);
	~ProcessOrphansTxt();

private:
	std::vector<ProcessOrphansTxtEntry *> m_entries;
};

