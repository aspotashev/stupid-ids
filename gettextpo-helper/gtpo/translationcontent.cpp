#include "gettextpo-helper.h"
#include "translationcontent.h"
#include "filecontentbase.h"
#include "filecontentfs.h"
#include "oidmapcache.h"
#include "stupids-client.h"
#include "message.h"
#include "block-sha1/sha1.h"
#include "filedatetime.h"
#include "messagegroup.h"

#include <git2.h>

#include <stdexcept>
#include <algorithm>
#include <iostream>

#include <stdio.h>
#include <string.h>


TranslationContent::TranslationContent(FileContentBase* fileContent)
    : m_fileContent(fileContent)
    , m_displayFilename("")
    , m_tphash(nullptr)
    , m_minIds()
    , m_minIdsInit(false)
    , m_messagesNormal()
    , m_messagesNormalInit(false)
    , m_date()
    , m_potDate()
    , m_author()
    , m_firstId(0) // 0 = uninitialized or undefined (undefined means that tp_hash is unknown on server)
    , m_idCount(-1)
{
    FileContentFs* file = dynamic_cast<FileContentFs*>(fileContent);
    if (file) {
        m_displayFilename = file->filename();
    }
}

TranslationContent::~TranslationContent()
{
    if (m_fileContent) {
        delete m_fileContent;
        m_fileContent = nullptr;
    }

    if (m_tphash) {
        delete m_tphash;
    }
}

void TranslationContent::setDisplayFilename(const std::string& filename)
{
    m_displayFilename = filename;
}

std::string TranslationContent::displayFilename() const
{
    return m_displayFilename;
}

po_file_t TranslationContent::poFileRead()
{
    assert(m_fileContent);

    return m_fileContent->poFileRead();
}

const git_oid *TranslationContent::gitBlobHash()
{
    assert(m_fileContent);

    return m_fileContent->gitBlobHash();
}

GitOid TranslationContent::getTpHash()
{
    if (m_tphash)
        return *m_tphash;

    // Cache calculated tp_hashes (it takes some time to
    // calculate a tp_hash). A singleton class is used for that.
    const git_oid *oid = m_fileContent ? m_fileContent->gitBlobHash() : nullptr;
    if (!oid)
        return GitOid::zero();

    GitOid tp_hash = TphashCache.getValue(oid);

    if (!tp_hash.isNull())
    {
        m_tphash = new GitOid(tp_hash);
    }
    else
    {
        std::string dump;

        try {
            dump = dumpPoFileTemplate();
        } catch (const ExceptionNotPo&) {
            return GitOid::zero();
        } catch (const ExceptionPoHeaderIncomplete&) {
            return GitOid::zero();
        }

        git_oid tphash_buf;
        sha1_buffer(&tphash_buf, dump.c_str(), dump.size());

        m_tphash = new GitOid(&tphash_buf);

        TphashCache.addPair(oid, &tphash_buf);
    }

    return *m_tphash;
}

std::string wrap_template_header(std::string headerString)
{
    if (!isalpha(headerString[0])) {
        // Header corrupt. See KDE SVN revision 1228594 for an example.
        throw TranslationContent::ExceptionNotPo();
    }

    // prepend "\n" to simplify search for "\nPOT-Creation-Date: "
    std::string header = std::string("\n") + headerString;

    // text that goes before the POT creation date
    std::string pot_creation_pattern("\nPOT-Creation-Date: ");

    // find the "POT-Creation-Date:" field
    size_t pot_creation_pos = header.find(pot_creation_pattern);
    if (pot_creation_pos == std::string::npos) {
        std::cerr << "header:\n[" << header << "]\n";

        // "POT-Creation-Date:" is missing from the header.
        throw TranslationContent::ExceptionPoHeaderIncomplete();
    }

    // extract the date
    pot_creation_pos += pot_creation_pattern.size(); // move to the beginning of the date
    size_t pot_date_end = header.find("\n", pot_creation_pos);
    assert(pot_date_end != std::string::npos); // There must be a "\n" after every line (including "POT-Creation-Date: ...") in the header.

    return wrap_string_hex(header.substr(pot_creation_pos, pot_date_end - pot_creation_pos).c_str());
}

std::string dump_filepos_entries(po_message_t message)
{
    std::string res;

    po_filepos_t filepos;
    char filepos_start_line[20];
    // FIXME: "filepos" entries should probably be sorted alphabetically
    // (because PO editors might reorder them).
    for (int i = 0; (filepos = po_message_filepos(message, i)); i ++)
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
    res += wrap_template_header(po_message_msgstr(message));

    // ordinary .po messages (not header)
    //
    // Assuming that PO editors do not change the order of messages.
    // Sorting messages in alphabetical order would be wrong, because for every template,
    // we store only the ID of the first message. The IDs of other messages should be deterministic.
    while ((message = po_next_message(iterator)))
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

void TranslationContent::readMessagesInternal(std::vector<MessageGroup*> &dest, bool &destInit)
{
    assert(!destInit);
    assert(dest.size() == 0);

    // m_displayFilename will be used as "filename" for all messages
    assert(!m_displayFilename.empty());

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
    while ((message = po_next_message(iterator)))
    {
        // Checking that obsolete messages go after all normal messages
//      assert(index == (int)dest.size());
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
        m_minIds = stupidsClient.getMinIds(getTpHash());
        m_minIdsInit = true;
        assertOk();
    }

    return m_minIds;
}

void TranslationContent::initFirstIdPair()
{
    if (m_firstId != 0)
        return;

    GitOid tp_hash = getTpHash();
    if (!tp_hash.isNull())
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

void TranslationContent::writeToFile(const std::string& destFilename, bool force_write)
{
    // Working with file
    po_file_t file = poFileRead();

    bool madeChanges = false;
    if (m_messagesNormalInit)
    {
        po_message_iterator_t iterator = po_message_iterator(file, "messages");

        // skipping header
        po_message_t message = po_next_message(iterator);

        for (size_t index = 0; (message = po_next_message(iterator)); )
        {
            if (po_message_is_obsolete(message))
                continue;

            assert(index < m_messagesNormal.size());
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

                        po_message_set_msgstr_plural(message, i, messageObj->msgstr(i).c_str());

                        // TODO: Linus Torvalds says I should fix my program.
                        // See linux-2.6/Documentation/CodingStyle
                    }

                    // Check that the number of plural forms has not changed
                    assert(po_message_msgstr_plural(message, messageObj->numPlurals()) == NULL);
                }
                else
                { // without plural forms
                    assert(messageObj->numPlurals() == 1);

                    OptString msg = messageObj->msgstr(0);
                    po_message_set_msgstr(message, msg.isNull() ? "" : msg.c_str());
                }

                OptString comments = messageObj->msgcomments();
                po_message_set_comments(message, comments.isNull() ? "" : comments.c_str());

                madeChanges = true;
            }

            index ++;
        }
        // free memory
        po_message_iterator_free(iterator);
    }

    if (force_write || madeChanges)
    {
//      libgettextpo_message_page_width_set(80);
        po_file_write(file, destFilename.c_str());
    }

    // free memory
    po_file_free(file);

    // TODO: set m_edited to "false" for all messages
}

void TranslationContent::writeToFile()
{
    FileContentFs* file = dynamic_cast<FileContentFs*>(m_fileContent);
    assert(file);

    writeToFile(file->filename(), false);
}

void TranslationContent::writeBufferToFile(const std::string& filename)
{
    assert(m_fileContent);

    m_fileContent->writeBufferToFile(filename);
}

void TranslationContent::assertOk()
{
    if (m_minIdsInit && m_messagesNormalInit)
        assert(m_minIds.size() == m_messagesNormal.size());
}

const FileDateTime& TranslationContent::date()
{
    if (!m_messagesNormalInit)
    {
        readMessagesInternal(m_messagesNormal, m_messagesNormalInit);
        assertOk();
    }

    return m_date;
}

const FileDateTime& TranslationContent::potDate()
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

void TranslationContent::setAuthor(const std::string& author)
{
    m_author = author;
}

MessageGroup *TranslationContent::findMessageGroupByOrig(const MessageGroup *msg)
{
    readMessages(); // "m_messagesNormal" should be initialized

    for (size_t i = 0; i < m_messagesNormal.size(); i ++)
        if (m_messagesNormal[i]->equalOrigText(*msg))
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

    // The former file content is not valid anymore
    if (m_fileContent) {
        delete m_fileContent;
        m_fileContent = nullptr;
    }

    m_date = from_content->date();

    std::vector<MessageGroup *> from = from_content->readMessages();
    // TBD: optimize this loop:
    // findMessageGroupByOrig() searches in O(log N), the whole loop
    // therefore takes N*log(N).
    // We can walk both catalogs in ascending order and find matches
    // in O(N), provided the catalogs are already sorted.
    for (size_t i = 0; i < from.size(); i ++)
    {
        MessageGroup *to = findMessageGroupByOrig(from[i]);
        if (to)
            to->updateTranslationFrom(from[i]);
        else
            assert(0); // not found in template, i.e. translation from "from_content" will be lost!
    }
}

void TranslationContent::clearTranslations()
{
    for (MessageGroup* msg : m_messagesNormal)
        msg->clearTranslation();
}

int TranslationContent::translatedCount() const
{
    return std::count_if(
        m_messagesNormal.begin(), m_messagesNormal.end(),
        [](MessageGroup* item) {
            return item->message(0)->isTranslated();
        });
}

//--------------------------------------------------

const char *TranslationContent::ExceptionPoHeaderIncomplete::what() const throw()
{
    return "ExceptionPoHeaderIncomplete (important information is missing from the .po file's header)";
}

//--------------------------------------------------

const char *TranslationContent::ExceptionNotPo::what() const throw()
{
    return "ExceptionNotPo (file is not a correct .po/.pot file)";
}

//--------------------------------------------------

std::vector<MessageGroup *> read_po_file_messages(const char *filename, bool loadObsolete)
{
    TranslationContent *content = new TranslationContent(new FileContentFs(filename));
    std::vector<MessageGroup *> res = content->readMessages();
    delete content;

    return res;
}
