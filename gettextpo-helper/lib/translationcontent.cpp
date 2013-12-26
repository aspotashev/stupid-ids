
#include <stdio.h>
#include <string.h>
#include <git2.h>

#include <gettextpo-helper/gettextpo-helper.h>
#include <gettextpo-helper/translationcontent.h>
#include <gettextpo-helper/oidmapcache.h>
#include <gettextpo-helper/stupids-client.h>
#include <gettextpo-helper/gitloader.h>
#include <gettextpo-helper/message.h>
#include <gettextpo-helper/sha1.h>
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
    case TYPE_DYNAMIC:
        assert(0); // There is no file to read, only "m_messagesNormal" contains valid data.
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

			delete m_oid;
			m_oid = NULL;
			return NULL;
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

	// Cache calculated tp_hashes (it takes some time to
	// calculate a tp_hash). A singleton class is used for that.
	const git_oid *oid = gitBlobHash();
	if (!oid)
		return NULL;

	const git_oid *tp_hash = TphashCache.getValue(oid);

	m_tphash = new git_oid;
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

std::string wrap_template_header(po_message_t message)
{
	char *header = new char [strlen(po_message_msgstr(message)) + 2];
	header[0] = '\n'; // prepend "\n" to simplify search for "\nPOT-Creation-Date: "
	strcpy(header + 1, po_message_msgstr(message));

	if (!isalpha(header[1]))
	{
		// Header corrupt. See KDE SVN revision 1228594 for an example.
		throw TranslationContent::ExceptionNotPo();
	}

	// find the "POT-Creation-Date:" field
	const char *pot_creation_pattern = "\nPOT-Creation-Date: "; // text that goes before the POT creation date
	char *pot_creation_str = strstr(header, pot_creation_pattern);
	if (pot_creation_str == NULL)
	{
		// There must be a "POT-Creation-Date:" in the header.

		printf("header:\n[%s]\n", header);
		assert(0);
	}

	// extract the date
	pot_creation_str += strlen(pot_creation_pattern); // move to the beginning of the date
	char *pot_date_end = strchr(pot_creation_str, '\n');
	assert(pot_date_end != NULL); // There must be a "\n" after every line (including "POT-Creation-Date: ...") in the header.
	*pot_date_end = '\0'; // truncate the string


	std::string res = wrap_string_hex(pot_creation_str);
	delete [] header;

	return res;
}

std::string dump_filepos_entries(po_message_t message)
{
	std::string res;

	po_filepos_t filepos;
	char filepos_start_line[20];
	// FIXME: "filepos" entries should probably be sorted alphabetically
	// (because PO editors might reorder them).
	for (int i = 0; filepos = po_message_filepos(message, i); i ++)
	{
		res += po_filepos_file(filepos);
		res += '\n';

		// Convert "po_filepos_start_line" to "int", because the return value is
		// (size_t)(-1) when no line number is available.
		sprintf(filepos_start_line, "%d", (int)po_filepos_start_line(filepos));
		res += filepos_start_line;
		res += '\n';
	}

	return res;
}

std::string dump_format_types(po_message_t message)
{
	std::string res;

	// FIXME: "*-format" entries should probably be sorted alphabetically
	// (because they might be reordered in other versions of "libgettext-po").
	for (int i = 0; po_format_list()[i] != NULL; i ++)
		if (po_message_is_format(message, po_format_list()[i]))
		{
			res += po_format_list()[i];
			res += '\n';
		}

	return res;
}

// include_non_id: include 'extracted comments', 'filepos entries', 'format types', 'range'
std::string wrap_template_message(po_message_t message, bool include_non_id)
{
	// header should be processed separately
	assert(*po_message_msgid(message) != '\0' || po_message_msgctxt(message) != NULL);

	if (po_message_is_obsolete(message)) // obsolete messages should not affect the dump
		return std::string();


	std::string res;

	res += "T"; // "T". May be NULL.
	res += wrap_string_hex(po_message_msgctxt(message));

	res += "M"; // "M". Cannot be NULL or empty.
	res += wrap_string_hex(po_message_msgid(message));

	res += "P"; // "P". May be NULL.
	res += wrap_string_hex(po_message_msgid_plural(message));

	if (!include_non_id)
		return res; // short dump used by "dump-ids"

	// additional fields (for calculation of "template-part hash")
	res += "C"; // "C". Cannot be NULL, may be empty.
	res += wrap_string_hex(po_message_extracted_comments(message));

	res += "N";
	res += wrap_string_hex(dump_filepos_entries(message).c_str());

	res += "F";
	res += wrap_string_hex(dump_format_types(message).c_str());

	int minp, maxp;
	if (po_message_is_range(message, &minp, &maxp)) // I have never seen POTs with ranges, but anyway...
	{
		char range_dump[30];
		sprintf(range_dump, "%xr%x", minp, maxp); // not characters, but integers are encoded in HEX here

		res += "R";
		res += range_dump;
	}


	return res;
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

    // TODO: function for this
    char *pot_date_str = po_header_field(po_file_domain_header(file, NULL), "POT-Creation-Date");
    if (pot_date_str)
    {
        m_potDate.fromString(pot_date_str);
        delete [] pot_date_str;
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

const std::vector<int> &TranslationContent::getMinIds()
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
	// Working with file
	po_file_t file = poFileRead();

    bool madeChanges = false;
    if (m_messagesNormalInit)
    {
        po_message_iterator_t iterator = po_message_iterator(file, "messages");

        // skipping header
        po_message_t message = po_next_message(iterator);

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

                        // TODO: Linus Torvalds says I should fix my program.
                        // See linux-2.6/Documentation/CodingStyle
                    }

                    // Check that the number of plural forms has not changed
                    assert(po_message_msgstr_plural(message, messageObj->numPlurals()) == NULL);
                }
                else
                { // without plural forms
                    assert(messageObj->numPlurals() == 1);

                    po_message_set_msgstr(message, messageObj->msgstr(0));
                }

                po_message_set_comments(message, messageObj->msgcomments() ? messageObj->msgcomments() : "");

                madeChanges = true;
            }

            index ++;
        }
        // free memory
        po_message_iterator_free(iterator);
    }

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

const FileDateTime &TranslationContent::potDate()
{
    if (!m_messagesNormalInit)
    {
        readMessagesInternal(m_messagesNormal, m_messagesNormalInit);
        assertOk();
    }

    return m_potDate;
}

std::string TranslationContent::author() const
{
	return m_author;
}

void TranslationContent::setAuthor(std::string author)
{
    m_author = author;
}

MessageGroup *TranslationContent::findMessageGroupByOrig(const MessageGroup *msg)
{
    readMessages(); // "m_messagesNormal" should be initialized

    for (size_t i = 0; i < m_messagesNormal.size(); i ++)
        if (m_messagesNormal[i]->equalOrigText(msg))
            return m_messagesNormal[i];

    return NULL;
}

void TranslationContent::copyTranslationsFrom(TranslationContent *from_content)
{
    // Read the current list of messages now, because we won't be able
    // read them after we set "m_type" to "TYPE_DYNAMIC".
    readMessages();

    // "m_tphash" is still valid
    // "m_minIdsInit", "m_firstId" and "m_idCount" are still valid

    // "m_oid" will (most likely) change
    if (m_oid)
    {
        delete m_oid;
        m_oid = NULL;
    }

    m_date = from_content->date();

    m_type = TYPE_DYNAMIC;
    m_gitLoader = NULL;
    m_buffer = NULL;
    m_bufferLen = 0;
    m_filename = NULL;


    std::vector<MessageGroup *> from = from_content->readMessages();
    for (size_t i = 0; i < from.size(); i ++)
    {
        MessageGroup *to = findMessageGroupByOrig(from[i]);
        if (to)
            to->updateTranslationFrom(from[i]);
        else
            assert(0); // not found in template, i.e. translation from "from_content" will be lost!
    }
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

