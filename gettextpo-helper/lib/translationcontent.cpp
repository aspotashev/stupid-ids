
#include <stdio.h>

#include <gettextpo-helper/gettextpo-helper.h>
#include <gettextpo-helper/translationcontent.h>

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
		printf("m_type = %d\n", m_type);
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

std::string TranslationContent::calculateTpHash()
{
	return sha1_string(dumpPoFileTemplate());
}

std::string TranslationContent::dumpPoFileTemplate()
{
	std::string res;

	po_file_t file = poFileRead(); // all checks and error reporting are done in poFileRead


	// main cycle
	po_message_iterator_t iterator = po_message_iterator(file, "messages");
	po_message_t message; // in fact, this is a pointer

	// processing header (header is the first message)
	message = po_next_message(iterator);
	res += wrap_template_header(message);

	// ordinary .po messages (not header)
	//
	// Assuming that PO editors do not change the order of messages.
	// Sorting messages in alphabetical order would be wrong, because for every template,
	// we store only the ID of the first message. The IDs of other messages should be deterministic.
	while (message = po_next_message(iterator))
	{
		std::string msg_dump = wrap_template_message(message, true);
		if (msg_dump.length() > 0) // non-obsolete
		{
			res += "/";
			res += msg_dump;
		}
	}

	// free memory
	po_message_iterator_free(iterator);
	po_file_free(file);

	return res;
}

std::vector<Message *> TranslationContent::readMessages(const char *filename, bool loadObsolete)
{
	po_file_t file = poFileRead();
	po_message_iterator_t iterator = po_message_iterator(file, "messages");

	// skipping header
	po_message_t message = po_next_message(iterator);

	std::vector<Message *> res;
	int index = 0;
	while (message = po_next_message(iterator))
	{
		// Checking that obsolete messages go after all normal messages
		assert(index == (int)res.size());

		if (loadObsolete || !po_message_is_obsolete(message))
			res.push_back(new Message(
				message,
				po_message_is_obsolete(message) ? -1 : index,
				filename));
		if (!po_message_is_obsolete(message))
			index ++;
	}

	// free memory
	po_message_iterator_free(iterator);
	po_file_free(file);

	return res;

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

