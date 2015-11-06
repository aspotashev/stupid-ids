#include "translation-collector.h"
#include "gettextpo-helper.h"
#include "stupids-client.h"
#include "translationcontent.h"
#include "filecontentfs.h"
#include "filecontentbuffer.h"
#include "message.h"
#include "gitloader.h"
#include "messagegroup.h"

#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <algorithm>

StupIdTranslationCollector::StupIdTranslationCollector()
{
    m_trans = std::map<int, MessageGroup*>();
    m_transInit = false;
}

StupIdTranslationCollector::~StupIdTranslationCollector()
{
}

void StupIdTranslationCollector::insertPo(const std::string& filename)
{
    TranslationContent *content = new TranslationContent(new FileContentFs(filename));
    insertPo(content); // takes ownership of "content"
}

// Takes ownership of "content"
void StupIdTranslationCollector::insertPo(TranslationContent *content)
{
    m_contents.push_back(content);
}

// Takes ownership of the buffer.
void StupIdTranslationCollector::insertPo(const void* buffer, size_t len, const std::string& filename)
{
    TranslationContent *content = new TranslationContent(new FileContentBuffer(buffer, len));
    content->setDisplayFilename(filename);
    insertPo(content); // takes ownership of "content"
}

void StupIdTranslationCollector::insertPoDir(const std::string& directory_path)
{
    // TBD: rewrite away from C
    DIR *dir = opendir(directory_path.c_str());
    struct dirent *entry;
    while ((entry = readdir(dir)))
    {
        std::string d_name(entry->d_name);
        std::string fullpath = directory_path + "/" + d_name;

        if (entry->d_type == DT_REG && ends_with(d_name.c_str(), ".po"))
            insertPo(fullpath);
        else if (entry->d_type == DT_DIR && !is_dot_or_dotdot(d_name))
            insertPoDir(fullpath);
    }

    closedir(dir);
}

bool file_exists(const std::string& filename)
{
    // TBD: rewrite away from C
    FILE *f = fopen(filename.c_str(), "r");
    if (f)
        fclose(f);

    return f != NULL;
}

void StupIdTranslationCollector::insertPoOrTemplate(
    const std::string& template_path, const std::string& translation_path)
{
    TranslationContent *content = NULL;

    if (file_exists(translation_path))
    {
        content = new TranslationContent(new FileContentFs(translation_path));
    }
    else
    {
        assert(file_exists(template_path));
        content = new TranslationContent(new FileContentFs(template_path));
        content->setDisplayFilename(translation_path);
    }

    insertPo(content); // takes ownership of "content"
}

void StupIdTranslationCollector::insertPoDirOrTemplate(
    const std::string& templates_path, const std::string& translations_path)
{
    // TBD: rewrite away from C
    DIR *dir = opendir(templates_path.c_str());
    struct dirent *entry;
    while ((entry = readdir(dir)))
    {
        std::string d_name(entry->d_name);
        std::string templ_subpath = templates_path + "/" + d_name;
        std::string trans_subpath = translations_path + "/" + d_name;

        if (entry->d_type == DT_REG && ends_with(d_name, ".pot"))
        {
            // .pot -> .po
            trans_subpath = trans_subpath.substr(0, trans_subpath.size() - 1);

            insertPoOrTemplate(templ_subpath, trans_subpath);
        }
        else if (entry->d_type == DT_DIR && !is_dot_or_dotdot(d_name))
            insertPoDirOrTemplate(templ_subpath, trans_subpath);
    }

    closedir(dir);
}

// Returns 'true' if there are different translations of the message.
//
// Cannot be 'const', because there is no const 'std::map::operator []'.
bool StupIdTranslationCollector::conflictingTrans(int min_id)
{
    initTransConfl();

    assert(m_trans[min_id]->size() > 0);

    Message *msg = m_trans[min_id]->message(0);
    for (int i = 1; i < m_trans[min_id]->size(); i ++)
        if (!msg->equalTranslationsComments(m_trans[min_id]->message(i)))
            return true;

    return false;
}

// Returns an array of 'min_id's.
//
// Cannot be 'const', because there is no const 'std::map::operator []'.
std::vector<int> StupIdTranslationCollector::listConflicting()
{
    initTransConfl();

    std::vector<int> res;

    for (std::map<int, MessageGroup *>::iterator iter = m_trans.begin();
        iter != m_trans.end();
        iter ++)
    {
        if (conflictingTrans(iter->first))
            res.push_back(iter->first);
    }

    return res;
}

MessageGroup *StupIdTranslationCollector::listVariants(int min_id)
{
    initTransConfl();

    assert (m_trans.find(min_id) != m_trans.end());

    return m_trans[min_id];
}

void StupIdTranslationCollector::initTransConfl()
{
    if (m_transInit)
        return;

    m_transInit = true;
    for (size_t content_i = 0; content_i < m_contents.size(); content_i ++)
    {
        TranslationContent *content = m_contents[content_i];

        std::vector<int> min_ids = content->getMinIds();

        //--------------------- insert messages --------------------
        std::vector<MessageGroup *> messages = content->readMessages();

        assert(messages.size() == min_ids.size());

        for (int index = 0; index < (int)messages.size(); index ++)
        {
            // fuzzy and untranslated messages will be also added
            if (m_trans.find(min_ids[index]) == m_trans.end())
            {
                std::pair<int, MessageGroup *> new_pair;
                new_pair.first = min_ids[index];
                new_pair.second = messages[index];

                m_trans.insert(new_pair);
            }
            else
            {
                m_trans[min_ids[index]]->mergeMessageGroup(messages[index]);
            }
        }
    }
}

std::vector<TranslationContent *> StupIdTranslationCollector::involvedByMinIds(std::vector<int> ids)
{
    std::vector<GitOid> tp_hashes;
    for (size_t i = 0; i < m_contents.size(); i ++)
        tp_hashes.push_back(m_contents[i]->calculateTpHash());

    std::vector<int> res_indices = stupidsClient.involvedByMinIds(tp_hashes, ids);

    std::vector<TranslationContent *> res;
    for (size_t i = 0; i < res_indices.size(); i ++)
    {
            size_t index = static_cast<size_t>(res_indices[i]);

            assert(index >= 0 && index < m_contents.size());
            res.push_back(m_contents[index]);
    }

    return res;
}

/**
 * \brief Generates the list of messages found in the contained TranslationContents by an array of IDs.
 */
// TODO: "std::vector<MessageGroup *>" -> "MessageGroup *" (MessageGroup may contain multiple translations)
void StupIdTranslationCollector::getMessagesByIds(
    std::map<int, std::vector<MessageGroup *> > &messages,
    std::vector<TranslationContent *> &contents,
    std::vector<int> ids)
{
    assert(messages.size() == 0);
    assert(contents.size() == 0);

    sort(ids.begin(), ids.end());

    contents = involvedByMinIds(ids);
    for (size_t i = 0; i < contents.size(); i ++)
    {
//      printf("Searching for interesting messages in %s\n", contents[i]->displayFilename());

        TranslationContent *content = contents[i];
        std::vector<MessageGroup *> content_msgs = content->readMessages();
        std::vector<int> content_ids = content->getMinIds();
                assert(content_msgs.size() == content_ids.size());

        for (size_t i = 0; i < content_msgs.size(); i ++)
            if (binary_search(ids.begin(), ids.end(), content_ids[i]))
                messages[content_ids[i]].push_back(content_msgs[i]);
    }
}

