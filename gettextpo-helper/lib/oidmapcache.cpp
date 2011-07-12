
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <map>
#include <algorithm>

#include <git2.h>

#include <gettextpo-helper/gettextpo-helper.h>

#include "oidmapcache.h"

std::map<std::string, OidMapCache *> OidMapCacheManager::s_instances = std::map<std::string, OidMapCache *>();

OidMapCache::OidMapCache(const char *filename)
{
	m_filename = xstrdup(filename);

	// Read the contents of the file
	FILE *f = fopen(m_filename, "a+b");
	assert(f);

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
		m_cache.push_back(std::pair<GitOid, GitOid>(GitOid(&oid), GitOid(&tp_hash)));
	}
	sort(m_cache.begin(), m_cache.end());

	fclose(f);
}

OidMapCache::~OidMapCache()
{
	delete [] m_filename;
}

const git_oid *OidMapCache::getValue(const git_oid *oid)
{
	std::vector<std::pair<GitOid, GitOid> >::iterator iter = lower_bound(
		m_cache.begin(), m_cache.end(), std::pair<GitOid, GitOid>(GitOid(oid), GitOid::zero()));
	if (iter != m_cache.end() && GitOid(oid) == iter->first)
		return iter->second.oid();
	else
		return NULL;
}

std::vector<GitOid> OidMapCache::reverseGetValues(const git_oid *oid)
{
	std::vector<GitOid> res;
	for (size_t i = 0; i < m_cache.size(); i ++)
		if (m_cache[i].second == GitOid(oid))
			res.push_back(m_cache[i].first);

	return res;
}

void OidMapCache::addPair(const git_oid *oid, const git_oid *tp_hash)
{
	// Check that we do not have this pair yet
	assert(getValue(oid) == NULL);

	// Add pair
	m_cache.push_back(std::pair<GitOid, GitOid>(GitOid(oid), GitOid(tp_hash)));
	sort(m_cache.begin(), m_cache.end());

	// Write into the file.
	// "a" (append) means that we will write to the end of file.
	FILE *f = fopen(m_filename, "ab");
	assert(f);

	assert(fwrite(oid, GIT_OID_RAWSZ, 1, f) == 1);
	assert(fwrite(tp_hash, GIT_OID_RAWSZ, 1, f) == 1);

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

