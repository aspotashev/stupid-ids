#include "gettextpo-helper.h"
#include "translationcontent.h"
#include "stupids-client.h"
#include "translation-collector.h"
#include "message.h"
#include "iddiff.h"
#include "iddiffmessage.h"
#include "messagegroup.h"

#include <git2.h>

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <set>

/**
 * \brief Constructs an empty iddiff
 */
Iddiff::Iddiff()
    : m_minimizedIds(false)
{
}

Iddiff::~Iddiff()
{
    clearIddiff();
}

void Iddiff::writeMessageList(rapidjson::PrettyWriter<rapidjson::StringBuffer> &writer,
                                const std::vector<IddiffMessage*>& list)
{
    writer.StartArray();
    for (auto x : list) {
        x->writeJson(writer);
    }
    writer.EndArray();
}

void Iddiff::clearIddiff()
{
    for (auto& kv : m_items) {
        delete kv.second;
    }

    m_items.clear();
}

void Iddiff::clearReviewComments()
{
    for (auto& kv : m_items) {
        kv.second->clearReviewComment();
    }
}

void Iddiff::clearReviewComment(int msg_id)
{
    if (m_items[msg_id]) {
        m_items[msg_id]->clearReviewComment();
    }
}

// This function fills m_addedItems.
// m_removedItems and m_reviewComments will be cleared.
void Iddiff::diffAgainstEmpty(TranslationContent *content_b)
{
    clearIddiff();

    GitOid tp_hash = content_b->calculateTpHash();
    int first_id = stupidsClient.getFirstId(tp_hash);
    assert(first_id > 0);


    // TODO: function for this
    m_date = content_b->date();
    m_author = content_b->author();

    // compare pairs of messages in 2 .po files
    po_file_t file_b = content_b->poFileRead();
    po_message_iterator_t iterator_b = po_message_iterator(file_b, "messages");
    // skipping headers
    po_message_t message_b = po_next_message(iterator_b);

    // TODO: use data from TranslationContent::readMessages
    for (int index = 0;
        (message_b = po_next_message(iterator_b)) &&
        !po_message_is_obsolete(message_b);
        index ++)
    {
        // Messages can be:
        //     "" -- untranslated (does not matter fuzzy or not, so f"" is illegal)
        //     "abc" -- translated
        //     f"abc" -- fuzzy

        // Types of possible changes:
        //     ""     -> ""     : -
        //     ""     -> "abc"  : ADDED
        //     ""     -> f"abc" : - (fuzzy messages are "weak", you should write in comments instead what you are not sure in)

        // Adding to "ADDED" if "B" is translated
        if (!po_message_is_untranslated(message_b) && !po_message_is_fuzzy(message_b))
            insertAdded(first_id + index, new IddiffMessage(message_b));
    }

    // free memory
    po_message_iterator_free(iterator_b);
    po_file_free(file_b);
}

// This function fills m_removedItems and m_addedItems
// m_reviewComments will be cleared.
void Iddiff::diffFiles(TranslationContent *content_a, TranslationContent *content_b)
{
    clearIddiff();

    // .po files should be derived from the same .pot
    GitOid tp_hash = content_a->calculateTpHash();
    assert(tp_hash == content_b->calculateTpHash());

    // first_id is the same for 2 files
    int first_id = stupidsClient.getFirstId(tp_hash);
    assert(first_id > 0);


    // TODO: function for this
    m_date = content_b->date();
    m_author = content_b->author();

    // compare pairs of messages in 2 .po files
//  po_file_t file_a = content_a->poFileRead();
//  po_file_t file_b = content_b->poFileRead();
//
//  po_message_iterator_t iterator_a =
//      po_message_iterator(file_a, "messages");
//  po_message_iterator_t iterator_b =
//      po_message_iterator(file_b, "messages");
//  // skipping headers
//  po_message_t message_a = po_next_message(iterator_a);
//  po_message_t message_b = po_next_message(iterator_b);
//
//  // TODO: use data from TranslationContent::readMessages
//  for (int index = 0;
//      (message_a = po_next_message(iterator_a)) &&
//      (message_b = po_next_message(iterator_b)) &&
//      !po_message_is_obsolete(message_a) &&
//      !po_message_is_obsolete(message_b);
//      index ++)
//  {

    std::vector<MessageGroup*> messages_a = content_a->readMessages();
    std::vector<MessageGroup*> messages_b = content_b->readMessages();
    for (size_t index = 0; index < messages_a.size(); index ++)
    {
        Message *message_a = messages_a[index]->message(0);
        Message *message_b = messages_b[index]->message(0);

//      if (strcmp(po_message_comments(message_a), po_message_comments(message_b)))
//      {
//          fprintf(stderr, "Changes in comments will be ignored!\n");
//          fprintf(stderr, "<<<<<\n");
//          fprintf(stderr, "%s", po_message_comments(message_a)); // "\n" should be included in comments
//          fprintf(stderr, "=====\n");
//          fprintf(stderr, "%s", po_message_comments(message_b)); // "\n" should be included in comments
//          fprintf(stderr, ">>>>>\n");
//      }


        // Messages can be:
        //     "" -- untranslated (does not matter fuzzy or not, so f"" is illegal)
        //     "abc" -- translated
        //     f"abc" -- fuzzy

        // bit 0 -- empty A
        // bit 1 -- empty B
        // bit 2 -- A == B (msgstr)
        // bit 3 -- fuzzy A
        // bit 4 -- fuzzy B
        #define FORMAT_IDDIFF_BITARRAY(p1, p2, p3, p4, p5, p6, p7, \
                                       p8, p9, p10, p11, p12, p13, p14) { \
            p1,  /* 00000    "abc" ->  "def"                                           */ \
            p2,  /* 00001    ""    ->  "abc"                                           */ \
            p3,  /* 00010    "abc" ->  ""                                              */ \
            p4,  /* 00011    ""    ->  "" (different nplurals)                         */ \
            p5,  /* 00100    "abc" ->  "abc"                                           */ \
            -1,  /* 00101   (empty cannot be equal to non-empty)                       */ \
            -1,  /* 00110   (empty cannot be equal to non-empty)                       */ \
            p6,  /* 00111       "" ->  ""                                              */ \
            p7,  /* 01000   f"abc" ->  "def"                                           */ \
            -1,  /* 01001   (empty+fuzzy A)                                            */ \
            p8,  /* 01010   f"abc" ->  ""                                              */ \
            -1,  /* 01011   (empty+fuzzy A, both empty but not equal)                  */ \
            p9,  /* 01100   f"abc" ->  "abc"                                           */ \
            -1,  /* 01101   (empty+fuzzy A, empty cannot be equal to non-empty)        */ \
            -1,  /* 01110   (empty cannot be equal to non-empty)                       */ \
            -1,  /* 01111   (empty+fuzzy A)                                            */ \
            p10, /* 10000    "abc" -> f"def"                                           */ \
            p11, /* 10001       "" -> f"abc"                                           */ \
            -1,  /* 10010   (empty+fuzzy B)                                            */ \
            -1,  /* 10011   (empty+fuzzy B, both empty, but not equal)                 */ \
            p12, /* 10100    "abc" -> f"abc"                                           */ \
            -1,  /* 10101   (empty cannot be equal to non-empty)                       */ \
            -1,  /* 10110   (empty+fuzzy B, empty cannot be equal to non-empty)        */ \
            -1,  /* 10111   (empty+fuzzy B)                                            */ \
            p13, /* 11000   f"abc" -> f"def"                                           */ \
            -1,  /* 11001   (empty+fuzzy A)                                            */ \
            -1,  /* 11010   (empty+fuzzy B)                                            */ \
            -1,  /* 11011   (empty+fuzzy A, empty+fuzzy B, both empty, but not equal)  */ \
            p14, /* 11100   f"abc" -> f"abc"                                           */ \
            -1,  /* 11101   (empty+fuzzy A, empty cannot be equal to non-empty)        */ \
            -1,  /* 11110   (empty+fuzzy B, empty cannot be equal to non-empty)        */ \
            -1,  /* 11111   (empty+fuzzy A, empty+fuzzy B)                             */ \
        }

        // Types of possible changes:
        // 00000    "abc"  -> "def"  : REMOVED, ADDED
        // 00001    ""     -> "abc"  : ADDED
        // 00010    "abc"  -> ""     : REMOVED
        // 00011    ""     -> ""     : - (different nplurals)
        // 00100    "abc"  -> "abc"  : -
        // 00111    ""     -> ""     : -
        // 01000    f"abc" -> "def"  : REMOVED, ADDED
        // 01010    f"abc" -> ""     : REMOVED (removing fuzzy messages is OK)
        // 01100    f"abc" -> "abc"  : ADDED
        // 10000    "abc"  -> f"def" : REMOVED
        // 10001    ""     -> f"abc" : - (fuzzy messages are "weak", you should write in comments instead what you are not sure in)
        // 10100    "abc"  -> f"abc" : REMOVED
        // 11000    f"abc" -> f"def" : - (fuzzy messages are "weak", you should write in comments instead what you are not sure in)
        // 11100    f"abc" -> f"abc" : -
        const int   needAdded[] = FORMAT_IDDIFF_BITARRAY(
            1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0);
        const int needRemoved[] = FORMAT_IDDIFF_BITARRAY(
            1, 0, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0);

        int indexCode = 0;
//      indexCode |= po_message_is_untranslated(message_a) ? 1 : 0;
//      indexCode |= po_message_is_untranslated(message_b) ? 2 : 0;
//      indexCode |= compare_po_message_msgstr(message_a, message_b) ? 0 : 4;
//      indexCode |= po_message_is_fuzzy(message_a) ? 8 : 0;
//      indexCode |= po_message_is_fuzzy(message_b) ? 16 : 0;
        indexCode |= message_a->isUntranslated() ? 1 : 0;
        indexCode |= message_b->isUntranslated() ? 2 : 0;
        indexCode |= message_a->equalMsgstr(message_b) ? 4 : 0;
        indexCode |= message_a->isFuzzy() ? 8 : 0;
        indexCode |= message_b->isFuzzy() ? 16 : 0;

        assert(indexCode >= 0 && indexCode < 32);
        assert(needRemoved[indexCode] != -1);
        assert(needAdded[indexCode] != -1);

        if (needRemoved[indexCode])
            insertRemoved(first_id + index, new IddiffMessage(*message_a));
        if (needAdded[indexCode])
            insertAdded(first_id + index, new IddiffMessage(*message_b));
    }

    // free memory
//  po_message_iterator_free(iterator_a);
//  po_message_iterator_free(iterator_b);
//  po_file_free(file_a);
//  po_file_free(file_b);
}

// http://www.infosoftcom.ru/article/realizatsiya-funktsii-split-string
//
// TBD: use a 3rd-party library, e.g.:
// http://www.boost.org/doc/libs/1_49_0/doc/html/boost/algorithm/split_id820181.html
// http://stackoverflow.com/questions/7930796/boosttokenizer-vs-boostsplit
std::vector<std::string> split_string(const std::string &str, const std::string &sep)
{
    int str_len = str.size();
    int sep_len = sep.size();
    assert(sep_len > 0);


    std::vector<std::string> res;

    int j = 0;
    int i = str.find(sep, j);
    while (i != -1)
    {
        if (i > j && i <= str_len)
            res.push_back(str.substr(j, i - j));
        j = i + sep_len;
        i = str.find(sep, j);
    }

    int l = str_len - 1;
    if (str.substr(j, l - j + 1).length() > 0)
        res.push_back(str.substr(j, l - j + 1));

    return res;
}

std::string join_strings(const std::vector<std::string> &str, const std::string &sep)
{
    std::string res;
    for (size_t i = 0; i < str.size(); i ++)
    {
        if (i > 0)
            res += sep;
        res += str[i];
    }

    return res;
}

// TODO: write and use function split_string()
std::set<std::string> set_of_lines(const std::string& input)
{
    std::set<std::string> res;

    std::stringstream ss(input);
    std::string item;
    while (std::getline(ss, item))
        res.insert(item);

    return res;
}

template <typename T>
std::vector<T> set_difference(const std::set<T> &a, const std::set<T> &b)
{
    std::vector<T> res(a.size());
    typename std::vector<T>::iterator it = set_difference(
        a.begin(), a.end(), b.begin(), b.begin(), res.begin());
    res.resize(it - res.begin());

    return res;
}

void Iddiff::diffTrCommentsAgainstEmpty(TranslationContent *content_b)
{
    clearIddiff();

    GitOid tp_hash = content_b->calculateTpHash();
    int first_id = stupidsClient.getFirstId(tp_hash);
    assert(first_id > 0);


    // TODO: function for this
    m_date = content_b->date();
    m_author = content_b->author();

    // compare pairs of messages in 2 .po files
    po_file_t file_b = content_b->poFileRead();
    po_message_iterator_t iterator_b = po_message_iterator(file_b, "messages");
    // skipping headers
    po_message_t message_b = po_next_message(iterator_b);

    // TODO: use data from TranslationContent::readMessages
    for (int index = 0;
        (message_b = po_next_message(iterator_b)) &&
        !po_message_is_obsolete(message_b);
        index ++)
    {
        std::set<std::string> comm_a; // empty
        std::set<std::string> comm_b = set_of_lines(po_message_comments(message_b));

        std::vector<std::string> added = set_difference(comm_b, comm_a);

        IddiffMessage *diff_message = new IddiffMessage();
        for (size_t i = 0; i < added.size(); i ++)
            diff_message->addMsgstr(added[i]);
        if (added.size() > 0)
            insertAdded(first_id + index, diff_message);
    }

    // free memory
    po_message_iterator_free(iterator_b);
    po_file_free(file_b);
}

void Iddiff::diffTrCommentsFiles(TranslationContent *content_a, TranslationContent *content_b)
{
    clearIddiff();

    // .po files should be derived from the same .pot
    GitOid tp_hash = content_a->calculateTpHash();
    assert(tp_hash == content_b->calculateTpHash());

    // first_id is the same for 2 files
    int first_id = stupidsClient.getFirstId(tp_hash);
    assert(first_id > 0);


    // TODO: function for this
    m_date = content_b->date();
    m_author = content_b->author();

    // compare pairs of messages in 2 .po files
    po_file_t file_a = content_a->poFileRead();
    po_file_t file_b = content_b->poFileRead();

    po_message_iterator_t iterator_a =
        po_message_iterator(file_a, "messages");
    po_message_iterator_t iterator_b =
        po_message_iterator(file_b, "messages");
    // skipping headers
    po_message_t message_a = po_next_message(iterator_a);
    po_message_t message_b = po_next_message(iterator_b);

    // TODO: use data from TranslationContent::readMessages
    for (int index = 0;
        (message_a = po_next_message(iterator_a)) &&
        (message_b = po_next_message(iterator_b)) &&
        !po_message_is_obsolete(message_a) &&
        !po_message_is_obsolete(message_b);
        index ++)
    {
        if (strcmp(po_message_comments(message_a), po_message_comments(message_b)) == 0)
            continue;

        std::set<std::string> comm_a = set_of_lines(po_message_comments(message_a));
        std::set<std::string> comm_b = set_of_lines(po_message_comments(message_b));

        std::vector<std::string> added = set_difference(comm_b, comm_a);

        IddiffMessage *diff_message = new IddiffMessage();
        for (size_t i = 0; i < added.size(); i ++)
            diff_message->addMsgstr(added[i]);
        if (added.size() > 0)
            insertAdded(first_id + index, diff_message);
    }

    // free memory
    po_message_iterator_free(iterator_a);
    po_message_iterator_free(iterator_b);
    po_file_free(file_a);
    po_file_free(file_b);
}

const FileDateTime &Iddiff::date() const
{
    return m_date;
}

std::string Iddiff::dateString() const
{
    return m_date.dateString();
}

std::string Iddiff::generateIddiffText()
{
    // TODO: rewrite without getRemovedVector/getAddedVector/getReviewVector to optimize for speed
    std::vector<std::pair<int, IddiffMessage *> > removed_list = getRemovedVector();
    std::vector<std::pair<int, IddiffMessage *> > added_list = getAddedVector();
    std::vector<std::pair<int, IddiffMessage *> > review_list = getReviewVector();

    if (removed_list.size() == 0 && added_list.size() == 0 && review_list.size() == 0) {
        return std::string("{}");
    }

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);

    writer.StartObject();

    writer.String("subject");
    writer.String(m_subject.c_str());

    writer.String("author");
    writer.String(m_author.c_str());

    writer.String("dateCreated");
    writer.String(dateString().c_str());

    std::set<int> ids;
    for (auto x : removed_list) {
        ids.insert(x.first);
    }
    for (auto x : added_list) {
        ids.insert(x.first);
    }
    for (auto x : review_list) {
        ids.insert(x.first);
    }

    writer.String("messages");
    writer.StartArray();
    for (int intId : ids) {
        // {
        //   "ref": {"intid": 123},
        //   "accept": ["abc", ...],
        //   "reject": ["def", ...],
        //   "review": ["bla bla bla", ...]
        // }
        writer.StartObject();

        writer.String("ref");
        writer.StartObject();
        writer.String("intid");
        writer.Int(intId);
        writer.EndObject();

        std::vector<IddiffMessage*> accept;
        std::vector<IddiffMessage*> reject;
        std::vector<IddiffMessage*> review;

        for (auto x : added_list) {
            if (intId == x.first) {
                accept.push_back(x.second);
            }
        }
        for (auto x : removed_list) {
            if (intId == x.first) {
                reject.push_back(x.second);
            }
        }
        for (auto x : review_list) {
            if (intId == x.first) {
                review.push_back(x.second);
            }
        }

        if (!accept.empty()) {
            writer.String("accept");
            writeMessageList(writer, accept);
        }
        if (!reject.empty()) {
            writer.String("reject");
            writeMessageList(writer, reject);
        }
        if (!review.empty()) {
            writer.String("review");
            writeMessageList(writer, review);
        }

        writer.EndObject();
    }
    writer.EndArray();

    writer.EndObject();

    return std::string(buffer.GetString());
}

bool Iddiff::loadIddiff(const char *filename)
{
    // TODO: function for reading the whole file
    FILE *f = fopen(filename, "r");
    if (!f)
        return false; // file does not exist

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    rewind(f);

    if (file_size <= 0)
    {
        fclose(f);
        return false; // file is empty
    }

    char *buffer = new char[file_size + 1];
    assert(fread(buffer, 1, file_size, f) == static_cast<size_t>(file_size));
    fclose(f);
    buffer[file_size] = '\0';


    rapidjson::Document doc;
    doc.Parse(buffer);

    assert(doc.IsObject());

    assert(doc.HasMember("subject"));
    m_subject = std::string(doc["subject"].GetString());

    assert(doc.HasMember("author"));
    m_author = std::string(doc["author"].GetString());

    if (doc.HasMember("dateCreated")) {
        m_date.fromString(doc["dateCreated"].GetString());
    }

    if (m_date.isNull()) {
        fprintf(stderr, "Warning: the .iddiff file does not have a \"Date:\" field in the header\n");
    }

    assert(doc.HasMember("messages"));
    const ValueType& messages = doc["messages"];
    assert(messages.IsArray());
    rapidjson::SizeType nMsg = messages.Size();

    // Check that the .iddiff is not useless
    assert(nMsg > 0);

    for (rapidjson::SizeType i = 0; i < nMsg; ++i) {
        const ValueType& msg = messages[i];
        assert(msg.IsObject());
        assert(msg.HasMember("ref"));

        assert(msg["ref"].HasMember("intid"));
        assert(msg["ref"]["intid"].IsInt());
        int intId = msg["ref"]["intid"].GetInt();

        // Check that this object is not useless
        assert(msg.HasMember("accept") || msg.HasMember("reject") || msg.HasMember("review"));

        if (msg.HasMember("accept")) {
            std::vector<IddiffMessage*> list = loadMessageList(msg["accept"]);
            for (auto x : list) {
                insertAdded(intId, x);
            }
        }

        if (msg.HasMember("reject")) {
            std::vector<IddiffMessage*> list = loadMessageList(msg["reject"]);
            for (auto x : list) {
                insertRemoved(intId, x);
            }
        }

        if (msg.HasMember("review")) {
            std::vector<IddiffMessage*> list = loadMessageList(msg["review"]);
            for (auto x : list) {
                insertReview(intId, x);
            }
        }
    }

    delete [] buffer;
    return true; // OK
}

void Iddiff::writeToFile(const char *filename)
{
    std::string text = generateIddiffText();
    if (!text.empty())
    {
        std::ofstream out(filename, std::ios::out | std::ios::trunc);
        out << text;
        out.close();
    }
}

/**
 * Allocates an object of class IddiffMessage
 */
std::pair<int, IddiffMessage *> Iddiff::loadMessageListEntry(const char *line)
{
    const char *space = strchr(line, ' ');
    assert(space);
    for (const char *cur = line; cur < space; cur ++)
        assert(isdigit(*cur));

    int msg_id = atoi(line);
    line = space + 1;

//  printf("%d <<<%s>>>\n", msg_id, line);

    IddiffMessage *msg = new IddiffMessage();
    char *msgstr_buf = new char[strlen(line) + 1];
    while (*line != '\0')
    {
        if (*line == 'f')
        {
            msg->setFuzzy(true);
            line ++;
        }

        assert(*line == '\"');
        line ++;

        int msgstr_i = 0;
        while (*line != '\0')
        {
            if (*line == '\"')
                break;
            else if (line[0] == '\\' && line[1] == '\"')
            {
                msgstr_buf[msgstr_i] = '\"';
                line ++;
            }
            else if (line[0] == '\\' && line[1] == 'n')
            {
                msgstr_buf[msgstr_i] = '\n';
                line ++;
            }
            else if (line[0] == '\\' && line[1] == 't')
            {
                msgstr_buf[msgstr_i] = '\t';
                line ++;
            }
            else if (line[0] == '\\' && line[1] == '\\')
            {
                msgstr_buf[msgstr_i] = '\\';
                line ++;
            }
            else
            {
                msgstr_buf[msgstr_i] = *line;
            }

            line ++;
            msgstr_i ++;
        }
        msgstr_buf[msgstr_i] = '\0';
        msg->addMsgstr(OptString(msgstr_buf));
//      printf("[[[%s]]]\n", msgstr_buf);

        assert(*line == '\"');
        line ++;
        if (*line == ' ')
            line ++;
        else
            assert(*line == '\0');
    }

    delete [] msgstr_buf;
    return std::pair<int, IddiffMessage*>(msg_id, msg);
}

// static
std::vector<IddiffMessage*> Iddiff::loadMessageList(const Iddiff::ValueType& array)
{
    rapidjson::SizeType nMsg = array.Size();

    // Check that the .iddiff is not useless
    assert(nMsg > 0);

    std::vector<IddiffMessage*> res;
    for (rapidjson::SizeType i = 0; i < nMsg; ++i) {
        // TODO: move this into class IddiffMessage
        const Iddiff::ValueType& msgTree = array[i];
        assert(msgTree.IsObject());

        assert(msgTree.HasMember("fuzzy"));
        assert(msgTree["fuzzy"].IsBool());

        IddiffMessage *msg = new IddiffMessage();
        msg->setFuzzy(msgTree["fuzzy"].GetBool());

        assert(msgTree.HasMember("msgstr"));
        const Iddiff::ValueType& msgstrTree = msgTree["msgstr"];
        assert(msgstrTree.IsArray());

        for (rapidjson::SizeType i = 0; i < msgstrTree.Size(); ++i) {
            assert(msgstrTree[i].IsString());
            msg->addMsgstr(OptString(msgstrTree[i].GetString()));
        }

        res.push_back(msg);
    }

    return res;
}

template <typename T>
void sort_uniq(std::vector<T> &v)
{
    sort(v.begin(), v.end());
    v.resize(unique(v.begin(), v.end()) - v.begin());
}

std::vector<int> Iddiff::involvedIds()
{
    std::vector<int> res;
    // TODO: macro for walking through an std::map
    for (const auto& kv : m_items) {
        res.push_back(kv.first);
    }

    sort_uniq(res);
    return res;
}

void Iddiff::cleanupMsgIdData(int msg_id)
{
    auto it = m_items.find(msg_id);
    if (it != m_items.end() && it->second->empty())
        delete it->second;
        m_items.erase(it);
}

void Iddiff::substituteMsgId(int old_id, int new_id)
{
    if (old_id == new_id)
        return;

    cleanupMsgIdData(old_id);
    cleanupMsgIdData(new_id);

    // Check that IDs won't collide
    if (m_items.find(new_id) != m_items.end())
    {
        // TODO: if there is no real conflict, this should not fail
        // (e.g. when the same string was changed in the same way via two different msg_IDs,
        // this often happens if you enable parallel branches in Lokalize)
        printf("new_id = %d, old_id = %d\n", new_id, old_id);
        assert(0);
    }

    if (m_items.find(old_id) != m_items.end())
    {
        m_items[new_id] = m_items[old_id]; // copying object of class IddiffChange
        m_items.erase(old_id);
    }
}

void Iddiff::minimizeIds()
{
    std::vector<int> msg_ids_arr = involvedIds();

    // Request minimized IDs from server
    std::vector<int> min_ids_arr = stupidsClient.getMinIds(msg_ids_arr);
    assert(msg_ids_arr.size() == min_ids_arr.size());

    for (size_t i = 0; i < msg_ids_arr.size(); i ++)
        substituteMsgId(msg_ids_arr[i], min_ids_arr[i]);

    m_minimizedIds = true;
}

// If the return value is true, we can put the new translation and drop the old one (when patching)
bool Iddiff::canDropMessage(const Message *message, int min_id)
{
    std::vector<IddiffMessage *> removed = findRemoved(min_id);

    if (message->isFuzzy() || message->isUntranslated())
        return true;

    for (size_t i = 0; i < removed.size(); i ++) // TODO: implement this through findRemoved (when Message and IddiffMessage will be merged)
        if (removed[i]->equalMsgstr(message))
            return true;
    return false;
}

void Iddiff::applyToMessage(MessageGroup *messageGroup, int min_id)
{
    assert(messageGroup->size() == 1);
    Message *message = messageGroup->message(0);
    IddiffMessage *added = findAddedSingle(min_id);

    if (canDropMessage(message, min_id))
    {
        if (added) // change translation
        {
            added->copyTranslationsToMessage(message);
            message->editFuzzy(false);
        }
        else // fuzzy old translation
        {
            //printf("Marking as FUZZY!!!\n");
            message->editFuzzy(true);
        }
    }
    else // !canDropMessage
    {
        // We cannot just add the new translation without
        // "blacklisting" the existing one.
        if (added && !added->equalTranslations(message))
        {
            std::cerr << "You have a conflict:" << std::endl <<
                " * Someone has already translated this message as:" << std::endl <<
                message->toJson() << std::endl <<
                " * And you are suggesting this:" << std::endl <<
                added->toJson() << std::endl;
            assert(0);
        }
    }
}

void Iddiff::applyToMessageComments(MessageGroup *messageGroup, int min_id)
{
    assert(messageGroup->size() == 1);
    Message *message = messageGroup->message(0);

    std::set<std::string> comments = set_of_lines(message->msgcomments());

    IddiffMessage *removed = findRemovedSingle(min_id);
    if (removed)
        for (int i = 0; i < removed->numPlurals(); i ++)
            comments.erase(std::string(removed->msgstr(i)));

    IddiffMessage *added = findAddedSingle(min_id);
    if (added)
        for (int i = 0; i < added->numPlurals(); i ++)
            comments.insert(std::string(added->msgstr(i)));


    std::vector<std::string> comments_vec;
    for (std::set<std::string>::iterator it = comments.begin();
         it != comments.end(); it ++)
         comments_vec.push_back(*it);
    // TODO: make join_strings accept any container, including std::set (or iterator?)
    std::string joined_comments = join_strings(comments_vec, std::string("\n"));

    message->editMsgcomments(joined_comments);
}

// TODO: remove this function!
// Applies the iddiff to the given TranslationContent and writes changes to file
void Iddiff::applyToContent(TranslationContent *content)
{
    std::vector<MessageGroup *> messages = content->readMessages();
    std::vector<int> min_ids = content->getMinIds();
    std::vector<int> involved_ids = involvedIds();

    for (size_t i = 0; i < messages.size(); i ++)
        if (binary_search(involved_ids.begin(), involved_ids.end(), min_ids[i]))
            applyToMessage(messages[i], min_ids[i]);
}

// TODO: Warn about messages that are involved in the Iddiff, but were not found in any of the .po files
void Iddiff::applyIddiff(StupIdTranslationCollector *collector, bool applyComments)
{
    // Check that involvedIds() will return minimized IDs
    assert(m_minimizedIds);

    std::map<int, std::vector<MessageGroup *> > messages;
    std::vector<TranslationContent *> contents;
    collector->getMessagesByIds(messages, contents, involvedIds());

    std::vector<int> involved_ids = involvedIds();
    for (size_t i = 0; i < involved_ids.size(); i ++)
    {
        int min_id = involved_ids[i];
        std::vector<MessageGroup *> messageGroups = messages[min_id];
        for (size_t j = 0; j < messageGroups.size(); j ++)
        {
            if (applyComments)
                applyToMessageComments(messageGroups[j], min_id);
            else
                applyToMessage(messageGroups[j], min_id);
        }

        if (messageGroups.size() == 0)
            fprintf(stderr,
                "Message has not been found nowhere by its ID (%d):\t%s\n",
                min_id, findAddedSingle(min_id)->toJson().c_str());
    }

    printf("involved contents: %d\n", (int)contents.size());
    for (size_t i = 0; i < contents.size(); i ++)
    {
        TranslationContent *content = contents[i];
        std::cout << "Writing " << content->displayFilename() << std::endl;
        content->writeToFile(content->displayFilename(), true); // TODO: StupIdTranslationCollector::writeChanges() for writing all changes to .po files
    }
}

void Iddiff::applyIddiff(StupIdTranslationCollector *collector)
{
    applyIddiff(collector, false);
}

void Iddiff::applyIddiffComments(StupIdTranslationCollector *collector)
{
    applyIddiff(collector, true);
}

/**
 * \brief Add translation version to the "REMOVED" section ensuring that it does not conflict with existing Iddiff entries.
 *
 * Takes ownership of "item".
 */
void Iddiff::insertRemoved(int msg_id, IddiffMessage *item)
{
    if (findRemoved(msg_id, item) != NULL) // duplicate in "REMOVED"
        return;

    assert(findAdded(msg_id, item) == NULL); // conflict: trying to "REMOVE" a translation already existing in "ADDED"

    if (!m_items[msg_id]) {
        m_items[msg_id] = new IddiffChange();
    }
    m_items[msg_id]->m_removedItems.push_back(item);
}

/**
 * \brief Add translation version to the "ADDED" section ensuring that it does not conflict with existing Iddiff entries.
 *
 * Takes ownership of "item".
 */
void Iddiff::insertAdded(int msg_id, IddiffMessage *item)
{
    if (findRemoved(msg_id, item) != NULL) // conflict: trying to "ADD" a translation already existing in "REMOVED"
    {
        fprintf(stderr,
            "Conflict: removing previously added translation\n"
            "\tmsg_id = %d\n"
            "\told message translation: %s\n"
            "\tnew message translation: %s\n",
            msg_id,
            findRemoved(msg_id, item)->toJson().c_str(),
            item->toJson().c_str());
        assert(0);
    }

    std::vector<IddiffMessage *> added_this_id = findAdded(msg_id);
    for (size_t i = 0; i < added_this_id.size(); i ++)
    {
        if (added_this_id[i]->equalMsgstr(item)) {
            // duplicate in "ADDED"
            return;
        } else {
            // conflict: two different translations in "ADDED"
            assert(0);
        }
    }

    if (!m_items[msg_id]) {
        m_items[msg_id] = new IddiffChange();
    }
    m_items[msg_id]->m_addedItems.push_back(item);
}

/**
 * \brief Add message to the "REVIEW" section ensuring that there was no "REVIEW" entry for the given msg_id.
 *
 * Takes ownership of "item".
 */
void Iddiff::insertReview(int msg_id, IddiffMessage *item)
{
    if (!m_items[msg_id]) {
        m_items[msg_id] = new IddiffChange();
    }

    assert(m_items[msg_id]->emptyReview());
    assert(item->isTranslated()); // text should not be empty

    m_items[msg_id]->m_reviewComment = item;
}

/**
 * Takes ownership of "item.second".
 */
void Iddiff::insertRemoved(std::pair<int, IddiffMessage *> item)
{
    insertRemoved(item.first, item.second);
}

/**
 * Takes ownership of "item.second".
 */
void Iddiff::insertAdded(std::pair<int, IddiffMessage *> item)
{
    insertAdded(item.first, item.second);
}

/**
 * Takes ownership of "item.second".
 */
void Iddiff::insertReview(std::pair<int, IddiffMessage *> item)
{
    insertReview(item.first, item.second);
}

/**
 * Does not take ownership of "item.second".
 */
void Iddiff::insertRemovedClone(std::pair<int, IddiffMessage *> item)
{
    insertRemoved(item.first, new IddiffMessage(*item.second));
}

/**
 * Does not take ownership of "item.second".
 */
void Iddiff::insertAddedClone(std::pair<int, IddiffMessage *> item)
{
    insertAdded(item.first, new IddiffMessage(*item.second));
}

/**
 * Does not take ownership of "item.second".
 */
void Iddiff::insertReviewClone(std::pair<int, IddiffMessage *> item)
{
    insertReview(item.first, new IddiffMessage(*item.second));
}

std::vector<std::pair<int, IddiffMessage *> > Iddiff::getRemovedVector()
{
    std::vector<std::pair<int, IddiffMessage *> > res;
    for (auto& kv : m_items) {
        std::vector<IddiffMessage *> id_items = kv.second->m_removedItems;
        for (size_t i = 0; i < id_items.size(); i ++) {
            res.push_back(std::pair<int, IddiffMessage *>(kv.first, id_items[i]));
        }
    }

    return res;
}

std::vector<std::pair<int, IddiffMessage *> > Iddiff::getAddedVector()
{
    std::vector<std::pair<int, IddiffMessage *> > res;
    for (auto& kv : m_items) {
        std::vector<IddiffMessage *> id_items = kv.second->m_addedItems;
        for (size_t i = 0; i < id_items.size(); i ++) {
            res.push_back(std::pair<int, IddiffMessage *>(kv.first, id_items[i]));
        }
    }

    return res;
}

std::vector<std::pair<int, IddiffMessage *> > Iddiff::getReviewVector()
{
    std::vector<std::pair<int, IddiffMessage *> > res;
    for (auto& kv : m_items) {
        if (kv.second->m_reviewComment) {
            res.push_back(std::pair<int, IddiffMessage *>(kv.first, kv.second->m_reviewComment));
        }
    }

    return res;
}

void Iddiff::mergeHeaders(Iddiff *diff)
{
    // Keep the most recent date/time
    if (m_date.isNull() || diff->date() > m_date) {
        m_date = diff->date();
    }

    std::vector<std::string> authors = split_string(m_author, std::string(", "));
    std::vector<std::string> added_authors = split_string(diff->m_author, std::string(", "));
    authors.insert(authors.end(), added_authors.begin(), added_authors.end());
    sort_uniq(authors);

    m_author = join_strings(authors, std::string(", "));
}

// TODO: Iddiffer::mergeNokeep that removes all items from the given Iddiff (and therefore it does not need to clone the IddiffMessage objects)
void Iddiff::merge(Iddiff *diff)
{
    std::vector<std::pair<int, IddiffMessage *> > other_removed = diff->getRemovedVector();
    std::vector<std::pair<int, IddiffMessage *> > other_added = diff->getAddedVector();
    std::vector<std::pair<int, IddiffMessage *> > other_review = diff->getReviewVector();

    for (size_t i = 0; i < other_removed.size(); i ++)
        insertRemovedClone(other_removed[i]); // this also clones the IddiffMessage object
    for (size_t i = 0; i < other_added.size(); i ++)
        insertAddedClone(other_added[i]); // this also clones the IddiffMessage object
    for (size_t i = 0; i < other_review.size(); i ++)
        insertReviewClone(other_review[i]); // this also clones the IddiffMessage object

    mergeHeaders(diff);
}

void Iddiff::mergeTrComments(Iddiff *diff)
{
    assert(diff->getReviewVector().empty());

    std::vector<std::pair<int, IddiffMessage *> > other_removed = diff->getRemovedVector();
    std::vector<std::pair<int, IddiffMessage *> > other_added = diff->getAddedVector();

    assert(other_removed.empty()); // TODO: implement later
//  for (size_t i = 0; i < other_removed.size(); i ++)
//      insertRemovedClone(other_removed[i]); // this also clones the IddiffMessage object

    for (size_t i = 0; i < other_added.size(); i ++)
    {
        int msg_id = other_added[i].first;
        IddiffMessage *message = other_added[i].second;

        IddiffMessage *prevMessage = findAddedSingle(msg_id);
        if (prevMessage)
        {
            std::vector<std::string> comments;
            for (int i = 0; i < message->numPlurals(); ++i)
                comments.push_back(std::string(message->msgstr(i)));
            for (int i = 0; i < prevMessage->numPlurals(); ++i)
                comments.push_back(std::string(prevMessage->msgstr(i)));

            sort_uniq(comments);

            IddiffMessage *new_message = new IddiffMessage();
            for (size_t i = 0; i < comments.size(); ++i)
                new_message->addMsgstr(comments[i]);

            eraseAdded(msg_id, prevMessage);
            insertAddedClone(std::make_pair(msg_id, new_message));
        }
        else
        {
            insertAddedClone(other_added[i]);
        }
    }

    mergeHeaders(diff);
}

std::vector<IddiffMessage *> Iddiff::findRemoved(int msg_id)
{
    // Avoiding addition of empty vector to m_removedItems
    if (m_items.find(msg_id) != m_items.end() && !m_items[msg_id]->emptyRemoved())
        return m_items[msg_id]->m_removedItems;
    else
        return std::vector<IddiffMessage *>();
}

std::vector<IddiffMessage *> Iddiff::findAdded(int msg_id)
{
    // Avoiding addition of empty vector to m_addedItems
    if (m_items.find(msg_id) != m_items.end() && !m_items[msg_id]->emptyAdded())
        return m_items[msg_id]->m_addedItems;
    else
        return std::vector<IddiffMessage *>();
}

IddiffMessage *Iddiff::findAddedSingle(int msg_id)
{
    std::vector<IddiffMessage *> arr = findAdded(msg_id);

    assert(arr.size() <= 1);
    return arr.size() == 1 ? arr[0] : NULL;
}

IddiffMessage *Iddiff::findRemovedSingle(int msg_id)
{
    std::vector<IddiffMessage *> arr = findRemoved(msg_id);

    assert(arr.size() <= 1);
    return arr.size() == 1 ? arr[0] : NULL;
}

/**
 * \static
 */
IddiffMessage *Iddiff::findIddiffMessageList(std::vector<IddiffMessage *> list, const IddiffMessage *item)
{
    for (size_t i = 0; i < list.size(); i ++)
        if (list[i]->equalMsgstr(item))
            return list[i];
    // There can't be duplicate items, so the first found item is the only matching one

    return NULL;
}

IddiffMessage *Iddiff::findRemoved(int msg_id, const IddiffMessage *item)
{
    return findIddiffMessageList(findRemoved(msg_id), item);
}

IddiffMessage *Iddiff::findRemoved(std::pair<int, IddiffMessage *> item)
{
    return findRemoved(item.first, item.second);
}

IddiffMessage *Iddiff::findAdded(int msg_id, const IddiffMessage *item)
{
    return findIddiffMessageList(findAdded(msg_id), item);
}

IddiffMessage *Iddiff::findAdded(std::pair<int, IddiffMessage *> item)
{
    return findAdded(item.first, item.second);
}

void Iddiff::eraseRemoved(int msg_id, const IddiffMessage *item)
{
    assert(m_items.find(msg_id) != m_items.end());

    m_items[msg_id]->eraseRemoved(item);
    cleanupMsgIdData(msg_id);
}

void Iddiff::eraseAdded(int msg_id, const IddiffMessage *item)
{
    assert(m_items.find(msg_id) != m_items.end());

    m_items[msg_id]->eraseAdded(item);
    cleanupMsgIdData(msg_id);
}

OptString Iddiff::reviewCommentText(int msg_id)
{
    IddiffMessage *comment = m_items[msg_id] ? m_items[msg_id]->m_reviewComment : nullptr;
    return comment ? comment->msgstr(0) : OptString(nullptr);
}

/**
 * Does not take ownership of "item".
 */
// 1. remove matching from m_removedItems (if any)
// 2. move all _other_ translations for this "msg_id" from m_addedItems to m_removedItems (there can be only one translation for this "msg_id" in m_addedItems)
// 3. add to m_addedItems
void Iddiff::acceptTranslation(int msg_id, const IddiffMessage *item)
{
    assert(msg_id);
    assert(item);

    IddiffMessage *removed = findRemoved(msg_id, item);
    if (removed)
        eraseRemoved(msg_id, removed);


    IddiffMessage *added = findAddedSingle(msg_id);
    if (!added)
    {
        // create new item
        insertAdded(msg_id, new IddiffMessage(*item));
    }
    else if (!added->equalMsgstr(item))
    {
        // move old item to "REMOVED", create new item
        insertAdded(msg_id, new IddiffMessage(*item));
        rejectTranslation(msg_id, added);
    }
}

/**
 * Does not take ownership of "item".
 */
// 1. remove matching from m_addedItems (if any)
// 2. add to m_removedItems
void Iddiff::rejectTranslation(int msg_id, const IddiffMessage *item)
{
    assert(msg_id);
    assert(item);

    IddiffMessage *added = findAddedSingle(msg_id);
    if (added && added->equalMsgstr(item))
        eraseAdded(msg_id, added);


    IddiffMessage *removed = findRemoved(msg_id, item);
    if (!removed)
        insertRemoved(msg_id, new IddiffMessage(*item));
}

/**
 * \brief Returns true if the "accept" action has already been reviewed.
 */
bool Iddiff::isAcceptAlreadyReviewed(int msg_id, IddiffMessage *item)
{
    return findAdded(msg_id, item) != NULL;
}

/**
 * \brief Returns true if the "reject" action has already been reviewed.
 */
bool Iddiff::isRejectAlreadyReviewed(int msg_id, IddiffMessage *item)
{
    return findRemoved(msg_id, item) != NULL;
}

void Iddiff::setCurrentDateTime()
{
    m_date.setCurrentDateTime();
}

// Why this is called "trusted" filtering:
//     If a string was in "ADDED" in "stupids-rerere",
//     the filter still accepts items from "input_diff" with
//     the same translation "REMOVED" (and vice versa).
//     I.e. trusted patches can overwrite data in "stupids-rerere".
//
// This is opposite to "3rd-party idpatch filtering".
void Iddiff::filterTrustedIddiff(Iddiff *filter, Iddiff *input_diff)
{
    if (!input_diff->getReviewVector().empty())
    {
        fprintf(stderr, "Warning: TODO: filter review items\n");
        assert(0);
    }

    std::vector<std::pair<int, IddiffMessage *> > other_removed = input_diff->getRemovedVector();
    std::vector<std::pair<int, IddiffMessage *> > other_added = input_diff->getAddedVector();

    for (size_t i = 0; i < other_removed.size(); i ++)
        if (!filter->findRemoved(other_removed[i]))
            insertRemovedClone(other_removed[i]);

    for (size_t i = 0; i < other_added.size(); i ++)
        if (!filter->findAdded(other_added[i]))
            insertAddedClone(other_added[i]);

    mergeHeaders(input_diff);
}
