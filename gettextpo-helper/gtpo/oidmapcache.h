
#include <string>
#include <map>

#include <gtpo/detectorbase.h>

// TODO: cache-*.dat files should have a header with:
// 1. format version number (what if we will want to change the format)
// 2. cache_id (for example "oid2tphash" in case of TphashCache)
class OidMapCache
{
public:
    OidMapCache(const std::string& filename);
    ~OidMapCache();

    void loadCache();
    void createDir(const std::string& pathname);
    void createPathDirectories();

    const git_oid *getValue(const git_oid *oid);
    std::vector<GitOid> reverseGetValues(const git_oid *oid);
    void addPair(const git_oid *oid, const git_oid *tp_hash);

private:
    std::string m_filename;
    bool m_fileExists;
    std::vector<std::pair<GitOid, GitOid> > m_cache;
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

