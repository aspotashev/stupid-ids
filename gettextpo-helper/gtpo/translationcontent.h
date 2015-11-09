#ifndef TRANSLATIONCONTENT_H
#define TRANSLATIONCONTENT_H

#include <gtpo/filedatetime.h>
#include <gtpo/gitoid.h>

#include <gettext-po.h>

#include <vector>
#include <string>

class MessageGroup;
class FileContentBase;

class TranslationContent
{
public:
    class ExceptionNotPo : public std::exception
    {
        virtual const char *what() const throw();
    };

    class ExceptionPoHeaderIncomplete : public std::exception
    {
        virtual const char *what() const throw();
    };

    /**
    * \brief Constructs a TranslationContent from a file content.
    *
    * \param fileContent File or git blob or buffer (FileContentFs/FileContentGit/FileContentBuffer).
    */
    TranslationContent(FileContentBase* fileContent);

    ~TranslationContent();

    /**
    * \brief Set the filename used for Message objects created by readMessages().
    */
    void setDisplayFilename(const std::string& filename);

    std::string displayFilename() const;

    // Caller should run 'po_file_free'
    po_file_t poFileRead();

    void writeBufferToFile(const std::string& filename);

    const git_oid *gitBlobHash();
    GitOid calculateTpHash();
    std::vector<MessageGroup *> readMessages();
    const FileDateTime& date();
    const FileDateTime& potDate();
    std::string author() const;
    void setAuthor(const std::string& author);

    const std::vector<int> &getMinIds();
    int getFirstId();
    int getIdCount();

    void writeToFile(const std::string& destFilename, bool force_write);
    void writeToFile();

    // Does the same as "msgmerge"
    void copyTranslationsFrom(TranslationContent *from);

    // Makes all messages untranslated
    void clearTranslations();

    int translatedCount() const;

private:
    // for calculateTpHash
    std::string dumpPoFileTemplate();

    void readMessagesInternal(std::vector<MessageGroup *>& dest, bool& destInit);

    void initFirstIdPair();

    void assertOk();

    MessageGroup* findMessageGroupByOrig(const MessageGroup* msg);

private:
    FileContentBase* m_fileContent;

    std::string m_displayFilename;

    // Cached tphash
    GitOid* m_tphash;

    std::vector<int> m_minIds;
    bool m_minIdsInit; // "true" if m_minIds is initialized

    // We use MessageGroup which may contain more than one translation,
    // but we actually use it here to store only one translation.
    std::vector<MessageGroup*> m_messagesNormal;
    bool m_messagesNormalInit;

    FileDateTime m_date;
    FileDateTime m_potDate;
    std::string m_author;

    int m_firstId;
    int m_idCount;
};

std::vector<MessageGroup *> read_po_file_messages(const char *filename, bool loadObsolete);

#endif // TRANSLATIONCONTENT_H
