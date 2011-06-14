
#ifndef TRANSLATIONCONTENT_H
#define TRANSLATIONCONTENT_H

#include <vector>
#include <string>

#include <gettext-po.h>
#include <git2.h>

class MessageGroup;
class GitLoaderBase;

class TranslationContent
{
public:
	class ExceptionNotPo : public std::exception
	{
		virtual const char *what() const throw();
	};

	TranslationContent(const char *filename);
	TranslationContent(GitLoaderBase *git_loader, const git_oid *oid);
	TranslationContent(const void *buffer, size_t len);
	~TranslationContent();

	void setDisplayFilename(const char *filename);
	const char *displayFilename() const;

	// Caller should run 'po_file_free'
	po_file_t poFileRead();

	const void *getDataBuffer();
	size_t getDataBufferLength();
	void writeBufferToFile(const char *filename);

	const git_oid *gitBlobHash();
	const git_oid *calculateTpHash();
	std::vector<MessageGroup *> readMessages(bool loadObsolete);

	std::vector<int> getMinIds();

	void writeToFile();

private:
	void clear();

	// Helpers for poFileRead()
	po_file_t poreadFile();
	po_file_t poreadGit();
	po_file_t poreadBuffer();

	// Helpers for getDataBuffer() and getDataBufferLength()
	void loadToBuffer();
	void loadToBufferFile();
	void loadToBufferGit();

	// for calculateTpHash
	std::string dumpPoFileTemplate();

	void readMessagesInternal(std::vector<MessageGroup *> &dest, bool &destInit, bool obsolete);

private:
	char *m_filename;
	char *m_displayFilename;

	GitLoaderBase *m_gitLoader;
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

