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

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <boost/date_time/local_time/local_time.hpp>

TranslationContent::TranslationContent(FileContentBase* fileContent)
    : m_fileContent(fileContent)
    , m_displayFilename("")
    , m_tphash(nullptr)
    , m_minIds()
    , m_minIdsInit(false)
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

po_file_t TranslationContent::poFileRead() const
{
    assert(m_fileContent);

    return m_fileContent->poFileRead();
}

const git_oid *TranslationContent::gitBlobHash()
{
    assert(m_fileContent);

    return m_fileContent->gitBlobHash();
}

GitOid TranslationContent::calculateTpHash() const
{
    std::string dump;

    try {
        dump = fileTemplateAsJson();
    } catch (const ExceptionNotPo&) {
        return GitOid::zero();
    } catch (const ExceptionPoHeaderIncomplete&) {
        return GitOid::zero();
    }

    git_oid tphash_buf;
    sha1_buffer(&tphash_buf, dump.c_str(), dump.size());

    return GitOid(&tphash_buf);
}

// static int64_t dateTimeAsUnixTimestamp(const std::string& str) {
//     std::istringstream ss("2011-07-06 04:15+0200");
//     ss.exceptions(std::ios_base::failbit);
//     boost::local_time::local_time_input_facet facet("%Y-%m-%d %H:%M%ZP");
//     ss.imbue(std::locale(ss.getloc(), &facet));
//     boost::local_time::local_date_time ldt(boost::date_time::not_a_date_time);
//     ss >> ldt;
// 
//     ldt.base_utc_offset();
// 
//     throw std::runtime_error(str);
// }

rapidjson::Value templateHeaderAsJson(std::string headerString, rapidjson::MemoryPoolAllocator<>& allocator)
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

    const std::string pot_creation_date_str = header.substr(pot_creation_pos, pot_date_end - pot_creation_pos);

    rapidjson::Value res;
    res.SetObject();

    {
        rapidjson::Value date;
        date.SetString(/*dateTimeAsUnixTimestamp(*/ pot_creation_date_str.c_str(), allocator);

        res.AddMember("pot_creation_date", date, allocator);
    }

    return res;
}

rapidjson::Value fileposEntriesAsJson(po_message_t message, rapidjson::MemoryPoolAllocator<>& allocator) {
    rapidjson::Value res;
    res.SetArray();

    po_filepos_t filepos;
//     char filepos_start_line[20];
    // FIXME: "filepos" entries should probably be sorted alphabetically
    // (because PO editors might reorder them).
    for (int i = 0; (filepos = po_message_filepos(message, i)); i++) {
        rapidjson::Value item;
        item.SetObject();

        {
            rapidjson::Value value;
            value.SetString(po_filepos_file(filepos), allocator);
            item.AddMember("file", value, allocator);
        }

        {
            // Convert "po_filepos_start_line" to "int", because the return value is
            // (size_t)(-1) when no line number is available.
            const int start_line = (int)po_filepos_start_line(filepos);

            if (start_line != -1) {
                rapidjson::Value value;
//                 value.SetIn(po_filepos_file(filepos), allocator);
                item.AddMember("line", start_line, allocator);
            }
        }

        res.PushBack(item, allocator);
    }

    return res;
}

rapidjson::Value messageformatsAsJson(po_message_t message, rapidjson::MemoryPoolAllocator<>& allocator) {
    rapidjson::Value res;
    res.SetArray();

    // FIXME: "*-format" entries should probably be sorted alphabetically
    // (because they might be reordered in other versions of "libgettext-po").
    for (int i = 0; po_format_list()[i] != NULL; i ++) {
        if (po_message_is_format(message, po_format_list()[i])) {
            rapidjson::Value value;
            value.SetString(po_format_list()[i], allocator);
            res.PushBack(value, allocator);
        }
    }

    return res;
}

static rapidjson::Value templateMessageAsString(
    po_message_t message, const bool includeTranslations, rapidjson::MemoryPoolAllocator<>& allocator)
{
    rapidjson::Value res;
    res.SetObject();

    // header should be processed separately
    if (*po_message_msgid(message) == '\0' && po_message_msgctxt(message) == NULL) {
        throw std::runtime_error("header");
    }

    if (po_message_is_obsolete(message)) { // obsolete messages should not affect the dump
        return rapidjson::Value();
    }

    {
        const char* str = po_message_msgctxt(message);
        if (str) {
            rapidjson::Value value;
            value.SetString(str, allocator);
            res.AddMember("msgctxt", value, allocator);
        }
    }

    {
        const char* str = po_message_msgid(message);
        if (str) {
            rapidjson::Value value;
            value.SetString(str, allocator);
            res.AddMember("msgid", value, allocator);
        }
    }

    {
        const char* str = po_message_msgid_plural(message);
        if (str) {
            rapidjson::Value value;
            value.SetString(str, allocator);
            res.AddMember("msgid_plural", value, allocator);
        }
    }

    {
        const char* str = po_message_extracted_comments(message);
        if (str) {
            rapidjson::Value value;
            value.SetString(str, allocator);
            res.AddMember("comments", value, allocator);
        }
    }

    res.AddMember("filepos", fileposEntriesAsJson(message, allocator), allocator);

    res.AddMember("formats", messageformatsAsJson(message, allocator), allocator);

    int minp, maxp;
    if (po_message_is_range(message, &minp, &maxp)) // I have never seen POTs with ranges, but anyway...
    {
        rapidjson::Value numeric_range;
        numeric_range.SetObject();
        
        numeric_range.AddMember("min", minp, allocator);
        numeric_range.AddMember("max", maxp, allocator);

        res.AddMember("numeric_range", numeric_range, allocator);
    }

    if (includeTranslations) {
        {
            // TBD: optimize - calling po_message_n_plurals() is not required
            int n_plurals = po_message_n_plurals(message);

            const bool plural = n_plurals > 0;

            if (n_plurals < 1)
                n_plurals = 1;

            rapidjson::Value msgstr_value;
            msgstr_value.SetArray();
            for (int i = 0; i < n_plurals; ++i) {
                const char *tmp;
                if (plural) {
                    tmp = po_message_msgstr_plural(message, i);
                }
                else {
                    assert(i == 0); // there can be only one form if 'm_plural' is false

                    tmp = po_message_msgstr(message);
                }

                rapidjson::Value value;
                value.SetString(tmp, allocator);
                msgstr_value.PushBack(value, allocator);
            }
            res.AddMember("msgstr", msgstr_value, allocator);
        }

        {
            rapidjson::Value value;
            const bool fuzzy = po_message_is_fuzzy(message) != 0;
            value.SetBool(fuzzy);
            res.AddMember("fuzzy", value, allocator);
        }

        {
            rapidjson::Value value;
            const bool obsolete = po_message_is_obsolete(message) != 0;
            value.SetBool(obsolete);
            res.AddMember("obsolete", value, allocator);
        }

        {
            rapidjson::Value value;
            const bool untranslated = po_message_is_untranslated(message);
            value.SetBool(untranslated);
            res.AddMember("untranslated", value, allocator);
        }

        {
            rapidjson::Value value;
            value.SetString(po_message_comments(message), allocator);
            res.AddMember("comments", value, allocator);
        }
    }

    return res;
}

std::string TranslationContent::asJson(const bool includeTranslations) const
{
    po_file_t file = poFileRead(); // all checks and error reporting are done in poFileRead
    if (!file) {
        std::stringstream ss;
        ss << "Failed to open file, errno = " << errno;
        throw std::runtime_error(ss.str());
    }

    // main cycle
    po_message_iterator_t iterator = po_message_iterator(file, "messages");
    po_message_t message; // in fact, this is a pointer

    // processing header (header is the first message)
    message = po_next_message(iterator);
    if (!message) { // no messages in file, i.e. not a .po/.pot file
        throw ExceptionNotPo();
    }

    rapidjson::MemoryPoolAllocator<> allocator;
    rapidjson::Document doc(&allocator);
    doc.SetObject();

    rapidjson::Value header_value = templateHeaderAsJson(po_message_msgstr(message), allocator);
    
    //=----------

    if (includeTranslations) {
        {
            char *date_str = po_header_field(po_file_domain_header(file, NULL), "PO-Revision-Date");
            if (date_str) {
                rapidjson::Value value;
                value.SetString(date_str, allocator);
                header_value.AddMember("po_revision_date", value, allocator);
            }
        }

    //     // TODO: function for this
    //     char *pot_date_str = po_header_field(po_file_domain_header(file, NULL), "POT-Creation-Date");
    //     if (pot_date_str) {
    //         res.potDate.fromString(pot_date_str);
    //         delete [] pot_date_str;
    //     }

        {
            char *author_str = po_header_field(po_file_domain_header(file, NULL), "Last-Translator");
            if (author_str) {
                rapidjson::Value value;
                value.SetString(author_str, allocator);
                header_value.AddMember("last_translator", value, allocator);
            }
        }
    }

    //-----------

    doc.AddMember("header", header_value, allocator);

    rapidjson::Value messages_value;
    messages_value.SetArray();

    // ordinary .po messages (not header)
    //
    // Assuming that PO editors do not change the order of messages.
    // Sorting messages in alphabetical order would be wrong, because for every template,
    // we store only the ID of the first message. The IDs of other messages should be deterministic.
    while ((message = po_next_message(iterator))) {
        rapidjson::Value msg_value = templateMessageAsString(message, includeTranslations, allocator);
        if (!msg_value.IsNull()) {
            messages_value.PushBack(msg_value, allocator);
        }
    }

    doc.AddMember("messages", messages_value, allocator);

    // free memory
    po_message_iterator_free(iterator);
    po_file_free(file);

    rapidjson::StringBuffer stream;
    rapidjson::Writer<rapidjson::StringBuffer> writer(stream);
    doc.Accept(writer);
    return stream.GetString();
}

TranslationContent::PoMessages TranslationContent::readMessagesInternal() const {
    // m_displayFilename will be used as "filename" for all messages
    assert(!m_displayFilename.empty());

    po_file_t file = poFileRead();
    po_message_iterator_t iterator = po_message_iterator(file, "messages");

    TranslationContent::PoMessages res;

    // TODO: function for this
    char *date_str = po_header_field(po_file_domain_header(file, NULL), "PO-Revision-Date");
    if (date_str)
    {
        res.date.fromString(date_str);
        delete [] date_str;
    }

    // TODO: function for this
    char *pot_date_str = po_header_field(po_file_domain_header(file, NULL), "POT-Creation-Date");
    if (pot_date_str)
    {
        res.potDate.fromString(pot_date_str);
        delete [] pot_date_str;
    }

    char *author_str = po_header_field(po_file_domain_header(file, NULL), "Last-Translator");
    if (author_str)
    {
        res.author = std::string(author_str);
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
            res.messages.push_back(new MessageGroup(
                message,
                po_message_is_obsolete(message) ? -1 : index,
                m_displayFilename));
        if (!po_message_is_obsolete(message))
            index ++;
    }

    // free memory
    po_message_iterator_free(iterator);
    po_file_free(file);

    return res;
}

std::vector<MessageGroup *> TranslationContent::readMessages() const {
    return readMessagesInternal().messages;
}

FileDateTime TranslationContent::date() const {
    return readMessagesInternal().date;
}

FileDateTime TranslationContent::potDate() const {
    return readMessagesInternal().potDate;
}

std::string TranslationContent::author() const {
    return readMessagesInternal().author;
}

const std::vector<int> &TranslationContent::getMinIds()
{
    if (!m_minIdsInit)
    {
        m_minIds = stupidsClient.getMinIds(getTpHash());
        m_minIdsInit = true;

        if (m_minIds.size() != readMessages().size()) {
            throw std::runtime_error("lengths of messages and m_minIds do not match");
        }
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

    po_message_iterator_t iterator = po_message_iterator(file, "messages");

    // skipping header
    po_message_t message = po_next_message(iterator);

    for (size_t index = 0; (message = po_next_message(iterator)); )
    {
        if (po_message_is_obsolete(message))
            continue;

        const auto messages = readMessages();
        assert(index < messages.size());
        MessageGroup *messageGroup = messages[index];

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

MessageGroup *TranslationContent::findMessageGroupByOrig(const MessageGroup *msg) {
    const auto messages = readMessages();
    for (size_t i = 0; i < messages.size(); i ++)
        if (messages[i]->equalOrigText(*msg))
            return messages[i];

    return NULL;
}

// void TranslationContent::copyTranslationsFrom(TranslationContent *from_content)
// {
//     // Read the current list of messages now, because we won't be able
//     // read them after we set "m_type" to "TYPE_DYNAMIC".
//     readMessages();
// 
//     // "m_tphash" is still valid
//     // "m_minIdsInit", "m_firstId" and "m_idCount" are still valid
// 
//     // The former file content is not valid anymore
//     if (m_fileContent) {
//         delete m_fileContent;
//         m_fileContent = nullptr;
//     }
// 
//     m_date = from_content->date();
// 
//     std::vector<MessageGroup *> from = from_content->readMessages();
//     // TBD: optimize this loop:
//     // findMessageGroupByOrig() searches in O(log N), the whole loop
//     // therefore takes N*log(N).
//     // We can walk both catalogs in ascending order and find matches
//     // in O(N), provided the catalogs are already sorted.
//     for (size_t i = 0; i < from.size(); i ++)
//     {
//         MessageGroup *to = findMessageGroupByOrig(from[i]);
//         if (to)
//             to->updateTranslationFrom(from[i]);
//         else
//             assert(0); // not found in template, i.e. translation from "from_content" will be lost!
//     }
// }

void TranslationContent::clearTranslations()
{
    for (MessageGroup* msg : readMessages())
        msg->clearTranslation();
}

int TranslationContent::translatedCount() const
{
    const auto messages = readMessages();
    return std::count_if(
        messages.begin(), messages.end(),
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
