
#include <vector>

class ProcessOrphansTxtEntry
{
public:
	enum
	{
		MOVE = 1,
		COPY = 2,
		DELETE = 4,
		MERGE = 8,
		MERGEKEEP = 16,
		ALL = MOVE | COPY | DELETE | MERGE | MERGEKEEP
	};


	ProcessOrphansTxtEntry(const char *cmd, const char *origin, const char *destination);
	~ProcessOrphansTxtEntry();

	int type() const;

//	const char *origin() const;
	const char *origNamePot() const;
	const char *origPath() const;

//	const char *destination() const;
	const char *destNamePot() const;
	const char *destPath() const;

protected:
	static void splitFullnamePot(const char *fullname, char **path, char **name);

private:
	int m_type;
	char *m_origin;
	char *m_destination;

	char *m_destPath;
	char *m_destNamePot;
	char *m_origPath;
	char *m_origNamePot;
};

//-------------------------------------------------------

class ProcessOrphansTxt
{
public:
	ProcessOrphansTxt(const char *filename);
	~ProcessOrphansTxt();

	void findByOrigin(std::vector<const ProcessOrphansTxtEntry *> &dest, const char *name, const char *path, int types) const;
	void findByDestination(std::vector<const ProcessOrphansTxtEntry *> &dest, const char *name, const char *path, int types) const;

private:
	std::vector<ProcessOrphansTxtEntry *> m_entries;
};

