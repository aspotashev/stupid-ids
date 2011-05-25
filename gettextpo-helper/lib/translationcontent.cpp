
#include "gettextpo-helper.h"
#include "translationcontent.h"

TranslationContent::TranslationContent(const char *filename)
{
	m_type = TYPE_FILE;
	m_gitLoader = NULL;

	m_filename = xstrdup(filename);
}

TranslationContent::~TranslationContent()
{
	if (m_filename)
		delete [] m_filename;
}

TranslationContent::TranslationContent(GitLoader *git_loader, const char *oid_str)
{
	m_type = TYPE_GIT;
	m_filename = NULL;

	m_gitLoader = git_loader;
	assert(git_oid_mkstr(&m_oid, oid_str) == GIT_SUCCESS);
}

po_file_t TranslationContent::poFileRead()
{
	switch (m_type)
	{
	case TYPE_FILE:
		return poreadFile();
	case TYPE_GIT:
		return poreadGit();
	default:
		assert(0);
		return NULL;
	}
}

po_file_t TranslationContent::poreadFile()
{
	assert(m_filename);

	return po_file_read(m_filename);
}

po_file_t TranslationContent::poreadGit()
{
	assert(m_gitLoader);

	git_blob *blob = m_gitLoader->blobLookup(&m_oid);
	if (!blob)
		return NULL;

	const void *rawcontent = git_blob_rawcontent(blob);
	int rawsize = git_blob_rawsize(blob);
	po_file_t file = po_buffer_read((const char *)rawcontent, (size_t)rawsize);

	git_blob_close(blob);

	return file;
}

//---------------------------------------------------------

GitLoader::GitLoader()
{
}

GitLoader::~GitLoader()
{
	for (size_t i = 0; i < m_repos.size(); i ++)
	{
		git_repository_free(m_repos[i]);
		m_repos[i] = NULL;
	}
}

git_blob *GitLoader::blobLookup(const git_oid *oid)
{
	git_blob *blob;

	for (size_t i = 0; i < m_repos.size(); i ++)
		if (git_blob_lookup(&blob, m_repos[i], oid) == 0)
			return blob;

	return NULL;
}

void GitLoader::addRepository(const char *git_dir)
{
	git_repository *repo;
	assert(git_repository_open(&repo, git_dir) == 0);

	m_repos.push_back(repo);
}

