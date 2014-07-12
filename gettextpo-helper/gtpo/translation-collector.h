
#include <stdlib.h>
#include <vector>
#include <map>

class Message;
class MessageGroup;
class TranslationContent;

class StupIdTranslationCollector
{
public:
    StupIdTranslationCollector();
    ~StupIdTranslationCollector();

    void insertPo(const std::string& filename);
    void insertPo(TranslationContent *content);
    void insertPo(const void *buffer, size_t len, const std::string& filename);
    void insertPoDir(const std::string& directory_path);

    void insertPoOrTemplate(const std::string& template_path, const std::string& translation_path);
    void insertPoDirOrTemplate(const std::string& templates_path, const std::string& translations_path);

    void initTransConfl();
    // Cannot be 'const', because there is no const 'std::map::operator []'.
    std::vector<int> listConflicting();
    MessageGroup *listVariants(int min_id);

    std::vector<TranslationContent *> involvedByMinIds(std::vector<int> ids);

    void getMessagesByIds(
        std::map<int, std::vector<MessageGroup *> > &messages,
        std::vector<TranslationContent *> &contents,
        std::vector<int> ids);

protected:
    // Cannot be 'const', because there is no const 'std::map::operator []'.
    bool conflictingTrans(int min_id);

private:
    std::vector<TranslationContent *> m_contents;

    // int -- min_id
    // std::vector<Message *> -- array of messages that belong to this 'min_id'
    std::map<int, MessageGroup *> m_trans;
    bool m_transInit; // "true" if m_trans is initialized
};

