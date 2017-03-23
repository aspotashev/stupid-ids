
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <map>
#include <algorithm>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#include <git2.h>

#include "gettextpo-helper.h"
#include "oidmapcache.h"

std::map<std::string, OidMapCache *> OidMapCacheManager::s_instances = std::map<std::string, OidMapCache *>();

OidMapCache::OidMapCache(const std::string& filename)
{
    m_filename = filename;
    m_fileExists = false;

    loadCache();
}

OidMapCache::~OidMapCache()
{
}

void OidMapCache::loadCache()
{
    assert(m_cache.empty());

    // Read the contents of the file
    FILE *f = fopen(m_filename.c_str(), "a+b");
    if (!f)
    {
        m_fileExists = false;
        return;
    }
    else
    {
        m_fileExists = true;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    rewind(f);

    assert(file_size % (2 * GIT_OID_RAWSZ) == 0);
    file_size /= 2 * GIT_OID_RAWSZ;

    git_oid oid;
    git_oid tp_hash;
    for (int i = 0; i < file_size; i ++)
    {
        assert(fread(&oid, GIT_OID_RAWSZ, 1, f) == 1);
        assert(fread(&tp_hash, GIT_OID_RAWSZ, 1, f) == 1);
        m_cache[GitOid(&oid)] = GitOid(&tp_hash);
    }

    fclose(f);
}

void OidMapCache::createDir(const std::string& pathname)
{
    DIR *dir = opendir(pathname.c_str());
    if (dir) // directory exists
        assert(closedir(dir) == 0);
    else if (errno == ENOENT)
        assert(mkdir(pathname.c_str(), 0777) == 0);
    else
        assert(0); // unexpected error
}

void OidMapCache::createPathDirectories()
{
    // "a/b" is the minimum directory+file name.
    if (m_filename.size() < 3)
        return;

    // TBD: rewrite without copying to C string
    char *filename = new char[m_filename.size() + 1];
    strcpy(filename, m_filename.c_str());

    // Not taking the root slash.
    char *cur_slash = filename;
    while ((cur_slash = strchr(cur_slash + 1, '/')))
    {
        *cur_slash = '\0';
        createDir(filename);
        *cur_slash = '/';
    }

    delete [] filename;
}

GitOid OidMapCache::getValue(const GitOid& oid) const
{
    const auto it = m_cache.find(oid);
    if (it != m_cache.cend())
        return it->second;
    else
        return GitOid::zero();
}

std::vector<GitOid> OidMapCache::reverseGetValues(const GitOid& oid) const
{
    std::vector<GitOid> res;
    for (const auto pr : m_cache) {
        if (pr.second == oid) {
            res.push_back(pr.first);
        }
    }

    return res;
}

void OidMapCache::addPair(const GitOid& oid, const GitOid& tp_hash)
{
    // Check that we do not have this pair yet
    assert(getValue(oid).isNull());

    // Add pair
    m_cache[oid] = tp_hash;

    if (!m_fileExists)
        createPathDirectories();

    // Write into the file.
    // "a" (append) means that we will write to the end of file.
    FILE *f = fopen(m_filename.c_str(), "ab");
    assert(f);

    assert(fwrite(oid.oid(), GIT_OID_RAWSZ, 1, f) == 1);
    assert(fwrite(tp_hash.oid(), GIT_OID_RAWSZ, 1, f) == 1);

    fclose(f);
}

//---------------------------------------------------

OidMapCacheManager::OidMapCacheManager()
{
}

OidMapCacheManager::~OidMapCacheManager()
{
    for (std::map<std::string, OidMapCache *>::iterator iter = s_instances.begin();
        iter != s_instances.end(); iter ++)
    {
        delete iter->second;
    }
}

// static
OidMapCache &OidMapCacheManager::instance(const char *cache_id)
{
    std::string cache_id_str(cache_id);
    std::map<std::string, OidMapCache *>::iterator iter = s_instances.find(cache_id_str);

    OidMapCache *cache = NULL;
    if (iter != s_instances.end())
    {
        cache = iter->second;
    }
    else
    {
        char *filename = new char[4096];
        sprintf(filename, "%s/.stupids/oidmapcache-%s.dat", getenv("HOME"), cache_id);
        assert(strlen(filename) < 4096);

        cache = new OidMapCache(filename);
        delete [] filename;

        s_instances[cache_id_str] = cache;
    }

    return *cache;
}
