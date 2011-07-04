
#include <stdio.h>
#include <string.h>

#include <gettextpo-helper/gettextpo-helper.h>
#include <gettextpo-helper/translationcontent.h>
#include <gettextpo-helper/oidmapcache.h>
#include <gettextpo-helper/stupids-client.h>
#include <gettextpo-helper/gitloader.h>
#include <gettextpo-helper/message.h>
#include "filedatetime.h"

TranslationContent::TranslationContent(const char *filename)
{
	clear();

	m_type = TYPE_FILE;
	m_filename = xstrdup(filename);
	setDisplayFilename(filename);
}

TranslationContent::TranslationContent(GitLoaderBase *git_loader, const git_oid *oid)
{
	clear();

	m_type = TYPE_GIT;
	m_gitLoader = git_loader;
	m_oid = new git_oid;
	git_oid_cpy(m_oid, oid);
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
	if (m_oid)
		delete m_oid;
	if (m_displayFilename)
		delete [] m_displayFilename;

	for (size_t i = 0; i < m_messagesNormal.size(); i ++)
		delete m_messagesNormal[i];
}

void TranslationContent::clear()
{
	m_type = TYPE_UNKNOWN;
	m_gitLoader = NULL;
	m_oid = NULL;
	m_buffer = NULL;
	m_bufferLen = 0;
	m_tphash = NULL;
	m_filename = NULL;
	m_displayFilename = NULL;

	m_minIdsInit = false;
	m_messagesNormalInit = false;
	m_firstId = 0; // 0 = uninitialized or undefined (undefined means that tp_hash is unknown on server)
	m_idCount = -1;
}

void TranslationContent::setDisplayFilename(const char *filename)
{
	if (m_displayFilename)
		delete [] m_displayFilename;

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

// TODO: use m_buffer if it is initialized
po_file_t TranslationContent::poreadGit()
{
	assert(m_gitLoader);
	assert(m_oid);

	git_blob *blob = m_gitLoader->blobLookup(m_oid);
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

	if (m_oid)
		return m_oid;

	// If type is TYPE_GIT, then m_oid must be already initialized

	m_oid = new git_oid;
	switch (m_type)
	{
	case TYPE_FILE:
		f = fopen(m_filename, "r");
		if (!f)
		{
			printf("Could not open file %s\n", m_filename);
			assert(0);
		}

		fseek(f, 0, SEEK_END);
		file_size = ftell(f);
		rewind(f);

		temp_buffer = new char[file_size];
		assert(fread(temp_buffer, 1, file_size, f) == file_size);
		fclose(f);

		assert(git_odb_hash(m_oid, temp_buffer, file_size, GIT_OBJ_BLOB) == 0);
		delete [] temp_buffer;
		break;
	case TYPE_BUFFER:
		assert(git_odb_hash(m_oid, m_buffer, m_bufferLen, GIT_OBJ_BLOB) == 0);
		break;
	default:
		printf("m_type = %d\n", m_type);
		assert(0);
		break;
	}

	return m_oid;
}

const git_oid *TranslationContent::calculateTpHash()
{
	if (m_tphash)
		return m_tphash;


	m_tphash = new git_oid;

	// Cache calculated tp_hashes (it takes some time to
	// calculate a tp_hash). A singleton class is used for that.
	const git_oid *oid = gitBlobHash();
	const git_oid *tp_hash = TphashCache.getValue(oid);

	if (tp_hash)
	{
		git_oid_cpy(m_tphash, tp_hash);
	}
	else
	{
		try
		{
			std::string dump = dumpPoFileTemplate();
			sha1_buffer(m_tphash, dump.c_str(), dump.size());
		}
		catch (std::exception &e)
		{
			return NULL;
		}

		TphashCache.addPair(oid, m_tphash);
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
	if (!message) // no messages in file, i.e. not a .po/.pot file
		throw ExceptionNotPo();
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

void TranslationContent::readMessagesInternal(std::vector<MessageGroup *> &dest, bool &destInit)
{
	assert(!destInit);
	assert(dest.size() == 0);

	// m_displayFilename will be used as "filename" for all messages
	assert(m_displayFilename);

	po_file_t file = poFileRead();
	po_message_iterator_t iterator = po_message_iterator(file, "messages");

	// TODO: function for this
	char *date_str = po_header_field(po_file_domain_header(file, NULL), "PO-Revision-Date");
	if (date_str)
	{
		m_date.fromString(date_str);
		delete [] date_str;
	}

	char *author_str = po_header_field(po_file_domain_header(file, NULL), "Last-Translator");
	if (author_str)
	{
		m_author = std::string(author_str);
		delete [] author_str;
	}

	// skipping header
	po_message_t message = po_next_message(iterator);

	int index = 0;
	while (message = po_next_message(iterator))
	{
		// Checking that obsolete messages go after all normal messages
//		assert(index == (int)dest.size());
		// TODO: check that! (the old check in the line above does not work after implementing caching)

		if (!po_message_is_obsolete(message))
			dest.push_back(new MessageGroup(
				message,
				po_message_is_obsolete(message) ? -1 : index,
				m_displayFilename));
		if (!po_message_is_obsolete(message))
			index ++;
	}

	// free memory
	po_message_iterator_free(iterator);
	po_file_free(file);

	destInit = true;
}

std::vector<MessageGroup *> TranslationContent::readMessages()
{
	if (!m_messagesNormalInit)
	{
		readMessagesInternal(m_messagesNormal, m_messagesNormalInit);
		assertOk();
	}

	return m_messagesNormal;
}

std::vector<int> TranslationContent::getMinIds()
{
	if (!m_minIdsInit)
	{
		m_minIds = stupidsClient.getMinIds(calculateTpHash());
		m_minIdsInit = true;
		assertOk();
	}

	return m_minIds;
}

void TranslationContent::initFirstIdPair()
{
	if (m_firstId != 0)
		return;

	const git_oid *tp_hash = calculateTpHash();
	if (tp_hash)
	{
		std::pair<int, int> res = stupidsClient.getFirstIdPair(tp_hash);
		m_firstId = res.first;
		m_idCount = res.second;
	}
}

int TranslationContent::getFirstId()
{
	initFirstIdPair();
	return m_firstId;
}

int TranslationContent::getIdCount()
{
	initFirstIdPair();
	return m_idCount;
}

void TranslationContent::writeToFile(const char *dest_filename, bool force_write)
{
	assert(m_messagesNormalInit);

	// Working with file
	po_file_t file = poFileRead();
	po_message_iterator_t iterator = po_message_iterator(file, "messages");

	// skipping header
	po_message_t message = po_next_message(iterator);

	bool madeChanges = false;
	for (int index = 0; message = po_next_message(iterator); )
	{
		if (po_message_is_obsolete(message))
			continue;

		assert(index >= 0 && index < m_messagesNormal.size());
		MessageGroup *messageGroup = m_messagesNormal[index];

		assert(messageGroup->size() == 1);
		Message *messageObj = messageGroup->message(0);

		// TODO: check that msgids have not changed

		if (messageObj->isEdited())
		{
			po_message_set_fuzzy(message, messageObj->isFuzzy() ? 1 : 0);
			if (po_message_msgid_plural(message))
			{ // with plural forms
				for (int i = 0; i < messageObj->numPlurals(); i ++)
				{
					// Check that the number of plural forms has not changed
					assert(po_message_msgstr_plural(message, i) != NULL);

					po_message_set_msgstr_plural(message, i, messageObj->msgstr(i));
				}

				// Check that the number of plural forms has not changed
				assert(po_message_msgstr_plural(message, messageObj->numPlurals()) == NULL);
			}
			else
			{ // without plural forms
				assert(messageObj->numPlurals() == 1);

				po_message_set_msgstr(message, messageObj->msgstr(0));
			}
			madeChanges = true;
		}

		index ++;
	}
	// free memory
	po_message_iterator_free(iterator);

	if (force_write || madeChanges)
	{
//		libgettextpo_message_page_width_set(80);
		po_file_write(file, dest_filename);
	}

	// free memory
	po_file_free(file);

	// TODO: set m_edited to "false" for all messages
}

void TranslationContent::writeToFile()
{
	assert(m_type == TYPE_FILE);
	writeToFile(m_filename, false);
}

const void *TranslationContent::getDataBuffer()
{
	if (!m_buffer) // if the data has not been buffered yet
		loadToBuffer();

	return m_buffer;
}

size_t TranslationContent::getDataBufferLength()
{
	if (!m_buffer) // if the data has not been buffered yet
		loadToBuffer();

	return m_bufferLen;
}

void TranslationContent::loadToBufferFile()
{
	assert(0); // not used for now, someone will write this later
}

void TranslationContent::loadToBufferGit()
{
	assert(m_gitLoader);
	assert(m_oid);

	git_blob *blob = m_gitLoader->blobLookup(m_oid);
	if (!blob)
		return;

	const void *rawcontent = git_blob_rawcontent(blob);
	int rawsize = git_blob_rawsize(blob);
	assert(rawsize > 0);

	char *rawcontent_copy = new char[rawsize];
	assert(rawcontent_copy);
	memcpy(rawcontent_copy, rawcontent, (size_t)rawsize);

	git_blob_close(blob);


	m_buffer = rawcontent_copy;
	m_bufferLen = (size_t)rawsize;
}

void TranslationContent::loadToBuffer()
{
	if (m_buffer) // if the data has already been buffered
		return;

	switch (m_type)
	{
	case TYPE_FILE:
		loadToBufferFile();
		break;
	case TYPE_GIT:
		loadToBufferGit();
		break;
	case TYPE_BUFFER:
		assert(0); // data should already be buffered (by definition of TYPE_BUFFER)
		break;
	default:
		printf("m_type = %d\n", m_type);
		assert(0);
	}
}

void TranslationContent::writeBufferToFile(const char *filename)
{
	loadToBuffer();

	FILE *f = fopen(filename, "w");
	assert(f);
	assert(fwrite(m_buffer, 1, m_bufferLen, f) == m_bufferLen);
	fclose(f);
}

void TranslationContent::assertOk()
{
	if (m_minIdsInit && m_messagesNormalInit)
		assert(m_minIds.size() == m_messagesNormal.size());
}

const FileDateTime &TranslationContent::date()
{
	if (!m_messagesNormalInit)
	{
		readMessagesInternal(m_messagesNormal, m_messagesNormalInit);
		assertOk();
	}

	return m_date;
}

std::string TranslationContent::author() const
{
	return m_author;
}

//--------------------------------------------------

const char *TranslationContent::ExceptionNotPo::what() const throw()
{
	return "ExceptionNotPo (file is not a correct .po/.pot file)";
}

//--------------------------------------------------

std::vector<MessageGroup *> read_po_file_messages(const char *filename, bool loadObsolete)
{
	TranslationContent *content = new TranslationContent(filename);
	std::vector<MessageGroup *> res = content->readMessages();
	delete content;

	return res;
}

