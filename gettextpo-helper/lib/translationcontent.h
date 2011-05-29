
#ifndef TRANSLATIONCONTENT_H
#define TRANSLATIONCONTENT_H

#include <gettext-po.h>
#include <git2.h>

class GitLoader
{
public:
	GitLoader();
	~GitLoader();

	git_blob *blobLookup(const git_oid *oid);
	void addRepository(const char *git_dir);

private:
	std::vector<git_repository *> m_repos;
};

class TranslationContent
{
public:
	TranslationContent(const char *filename);
	TranslationContent(GitLoader *git_loader, const char *oid_str);
	TranslationContent(const void *buffer, size_t len);
	~TranslationContent();

	// Caller should run 'po_file_free'
	po_file_t poFileRead();

	std::string calculateTpHash();
	std::vector<Message *> readMessages(const char *filename, bool loadObsolete);

protected:
	po_file_t poreadFile();
	po_file_t poreadGit();
	po_file_t poreadBuffer();

	// for calculateTpHash
	std::string dumpPoFileTemplate();

private:
	char *m_filename;

	GitLoader *m_gitLoader;
	git_oid m_oid;

	const void *m_buffer;
	size_t m_bufferLen;

	int m_type;

	enum
	{
		TYPE_FILE = 1,
		TYPE_GIT = 2,
		TYPE_BUFFER = 3
	};
};

#endif // TRANSLATIONCONTENT_H

