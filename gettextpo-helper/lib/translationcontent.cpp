
#include <stdio.h>

#include <gettextpo-helper/gettextpo-helper.h>
#include <gettextpo-helper/translationcontent.h>
#include <gettextpo-helper/tphashcache.h>

TranslationContent::TranslationContent(const char *filename)
{
	clear();

	m_type = TYPE_FILE;
	m_filename = xstrdup(filename);
}

TranslationContent::TranslationContent(GitLoader *git_loader, const char *oid_str)
{
	clear();

	m_type = TYPE_GIT;
	m_gitLoader = git_loader;
	assert(git_oid_mkstr(&m_oid, oid_str) == GIT_SUCCESS);
}

TranslationContent::TranslationContent(const void *buffer, size_t len)
{
	clear();

	m_type = TYPE_BUFFER;
	m_buffer = buffer; // take ownership of the buffer
	m_bufferLen = len;
}

TranslationContent::~TranslationContent()
{
	if (m_filename)
		delete [] m_filename;
	if (m_buffer)
		delete [] (char *)m_buffer;
	if (m_tphash)
		delete m_tphash;
	if (m_displayFilename)
		delete [] m_displayFilename;
}

void TranslationContent::clear()
{
	m_type = TYPE_UNKNOWN;
	m_gitLoader = NULL;
	m_buffer = NULL;
	m_bufferLen = 0;
	m_tphash = NULL;
	m_filename = NULL;
	m_displayFilename = NULL;
}

void TranslationContent::setDisplayFilename(const char *filename)
{
	assert(m_displayFilename == NULL); // can be set only once

	m_displayFilename = xstrdup(filename);
}

const char *TranslationContent::displayFilename() const
{
	return m_displayFilename;
}

po_file_t TranslationContent::poFileRead()
{
	switch (m_type)
	{
	case TYPE_FILE:
		return poreadFile();
	case TYPE_GIT:
		return poreadGit();
	case TYPE_BUFFER:
		return poreadBuffer();
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

po_file_t TranslationContent::poreadBuffer()
{
	assert(m_buffer);

	return po_buffer_read((const char *)m_buffer, m_bufferLen);
}

const git_oid *TranslationContent::gitBlobHash()
{
	char *temp_buffer;
	long file_size;
	FILE *f;

	switch (m_type)
	{
	case TYPE_FILE:
		f = fopen(m_filename, "r");
		fseek(f, 0, SEEK_END);
		file_size = ftell(f);
		rewind(f);

		temp_buffer = new char[file_size];
		assert(fread(temp_buffer, 1, file_size, f) == file_size);
		fclose(f);

		assert(git_odb_hash(&m_oid, temp_buffer, file_size, GIT_OBJ_BLOB) == 0);
		delete [] temp_buffer;

		return &m_oid;
//	case TYPE_GIT:
//		return poreadGit();
	case TYPE_BUFFER:
		assert(git_odb_hash(&m_oid, m_buffer, m_bufferLen, GIT_OBJ_BLOB) == 0);
		return &m_oid;
	default:
		printf("m_type = %d\n", m_type);
		assert(0);
		return NULL;
	}
}

const git_oid *TranslationContent::calculateTpHash()
{
	if (m_tphash)
		return m_tphash;


	m_tphash = new git_oid;

	// Cache calculated tp_hashes (it takes some time to
	// calculate a tp_hash). A singleton class is used for that.
	const git_oid *oid = gitBlobHash();
	const git_oid *tp_hash = TphashCache::getTphash(oid);

	if (tp_hash)
	{
		git_oid_cpy(m_tphash, tp_hash);
	}
	else
	{
		std::string tphash_str = sha1_string(dumpPoFileTemplate());
		assert(git_oid_mkstr(m_tphash, tphash_str.c_str()) == GIT_SUCCESS);

		TphashCache::addPair(oid, m_tphash);
	}

	return m_tphash;
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

std::vector<MessageGroup *> TranslationContent::readMessages(const char *filename, bool loadObsolete)
{
	po_file_t file = poFileRead();
	po_message_iterator_t iterator = po_message_iterator(file, "messages");

	// skipping header
	po_message_t message = po_next_message(iterator);

	std::vector<MessageGroup *> res;
	int index = 0;
	while (message = po_next_message(iterator))
	{
		// Checking that obsolete messages go after all normal messages
		assert(index == (int)res.size());

		if (loadObsolete || !po_message_is_obsolete(message))
			res.push_back(new MessageGroup(
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

