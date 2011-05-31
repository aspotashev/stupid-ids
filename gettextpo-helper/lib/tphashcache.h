
#include <gettextpo-helper/detectorbase.h>

class TphashCache
{
public:
	static const git_oid *getTphash(const git_oid *oid);
	static void addPair(const git_oid *oid, const git_oid *tp_hash);

private:
	TphashCache();
	~TphashCache();
	const git_oid *doGetTphash(const git_oid *oid);
	void doAddPair(const git_oid *oid, const git_oid *tp_hash);

	static TphashCache &instance();

	static TphashCache *s_instance;

	char *m_filename;
	std::map<GitOid, GitOid> m_cache;
};

