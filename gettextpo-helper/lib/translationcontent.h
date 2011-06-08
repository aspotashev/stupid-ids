
#ifndef TRANSLATIONCONTENT_H
#define TRANSLATIONCONTENT_H

#include <vector>
#include <string>

#include <gettext-po.h>
#include <git2.h>

class MessageGroup;
class GitLoader;

class TranslationContent
{
public:
	TranslationContent(const char *filename);
	TranslationContent(GitLoader *git_loader, const git_oid *oid);
	TranslationContent(const void *buffer, size_t len);
	~TranslationContent();

	void setDisplayFilename(const char *filename);
	const char *displayFilename() const;

	// Caller should run 'po_file_free'
	po_file_t poFileRead();

	const git_oid *gitBlobHash();
	const git_oid *calculateTpHash();
	std::vector<MessageGroup *> readMessages(bool loadObsolete);

	std::vector<int> getMinIds();

	void writeToFile();

private:
	void clear();

	po_file_t poreadFile();
	po_file_t poreadGit();
	po_file_t poreadBuffer();

	// for calculateTpHash
	std::string dumpPoFileTemplate();

	void readMessagesInternal(std::vector<MessageGroup *> &dest, bool &destInit, bool obsolete);

private:
	char *m_filename;
	char *m_displayFilename;

	GitLoader *m_gitLoader;
	git_oid *m_oid;
	git_oid *m_tphash;

	const void *m_buffer;
	size_t m_bufferLen;

	int m_type;

	std::vector<int> m_minIds;
	bool m_minIdsInit; // "true" if m_minIds is initialized

	std::vector<MessageGroup *> m_messagesNormal;
	bool m_messagesNormalInit;

	enum
	{
		TYPE_UNKNOWN = 0,
		TYPE_FILE = 1,
		TYPE_GIT = 2,
		TYPE_BUFFER = 3
	};
};

#endif // TRANSLATIONCONTENT_H

