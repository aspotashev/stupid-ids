
#include <string>

#include <gettextpo-helper/detectorbase.h>

class OidMapCache
{
public:
	OidMapCache(const char *filename);
	~OidMapCache();

	// TODO: rename this function ("getValue"?)
	const git_oid *getTphash(const git_oid *oid);
	void addPair(const git_oid *oid, const git_oid *tp_hash);

private:
	char *m_filename;
	std::map<GitOid, GitOid> m_cache;
};

//---------------------------------------------------

class OidMapCacheManager
{
public:
	OidMapCacheManager();
	~OidMapCacheManager();

	static OidMapCache &instance(const char *cache_id);

private:
	static std::map<std::string, OidMapCache *> s_instances;
};

//---------------------------------------------------

#define TphashCache (OidMapCacheManager::instance("oid2tphash"))

