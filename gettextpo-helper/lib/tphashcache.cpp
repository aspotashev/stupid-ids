
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <map>

#include <git2.h>

#include "tphashcache.h"

TphashCache *TphashCache::s_instance = NULL;

TphashCache::TphashCache()
{
	const char *home = getenv("HOME");
	const char *local_filename = "/.stupids/tphashcache.dat";

	m_filename = new char[strlen(home) + strlen(local_filename) + 1];
	strcpy(m_filename, home);
	strcat(m_filename, local_filename);

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
		m_cache[GitOid(&oid)] = GitOid(&tp_hash);
	}

	fclose(f);
}

TphashCache::~TphashCache()
{
	delete [] m_filename;
}

TphashCache &TphashCache::instance()
{
	if (s_instance == NULL)
	{
		s_instance = new TphashCache();
	}

	return *s_instance;
}

const git_oid *TphashCache::getTphash(const git_oid *oid)
{
	return instance().doGetTphash(oid);
}

void TphashCache::addPair(const git_oid *oid, const git_oid *tp_hash)
{
	instance().doAddPair(oid, tp_hash);
}

const git_oid *TphashCache::doGetTphash(const git_oid *oid)
{
	std::map<GitOid, GitOid>::iterator iter = m_cache.find(GitOid(oid));
	if (iter == m_cache.end())
		return NULL;
	else
		return iter->second.oid();
}

void TphashCache::doAddPair(const git_oid *oid, const git_oid *tp_hash)
{
	// Check that we do not have this pair yet
	assert(doGetTphash(oid) == NULL);

	// Add pair
	m_cache[GitOid(oid)] = GitOid(tp_hash);

	// Write into the file.
	// "a" (append) means that we will write to the end of file.
	FILE *f = fopen(m_filename, "ab");
	assert(f);

	assert(fwrite(oid, GIT_OID_RAWSZ, 1, f) == 1);
	assert(fwrite(tp_hash, GIT_OID_RAWSZ, 1, f) == 1);

	fclose(f);
}

