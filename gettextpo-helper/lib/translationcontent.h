
#ifndef TRANSLATIONCONTENT_H
#define TRANSLATIONCONTENT_H

#include <vector>
#include <string>

#include <gettext-po.h>
#include <git2.h>
#include <gettextpo-helper/filedatetime.h>

class MessageGroup;
class GitLoaderBase;

class TranslationContent
{
public:
    class ExceptionNotPo : public std::exception
    {
        virtual const char *what() const throw();
    };

    /**
    * \brief Constructs a TranslationContent from a file.
    *
    * \param filename File name.
    */
    TranslationContent(const char *filename);

    /**
    * \brief Constructs a TranslationContent from a Git blob identified by its OID.
    *
    * \param git_loader Git repositories list used for searching the blob by OID.
    * \param oid OID of the blob.
    */
    TranslationContent(GitLoaderBase *git_loader, const git_oid *oid);

    /**
    * \brief Constructs a TranslationContent using data from a buffer.
    *
    * \param buffer Buffer.
    * \param len Size of the buffer.
    */
    TranslationContent(const void *buffer, size_t len);

    ~TranslationContent();

    /**
    * \brief Set the filename used for Message objects created by readMessages().
    */
    void setDisplayFilename(const char *filename);

    const char *displayFilename() const;

    // Caller should run 'po_file_free'
    po_file_t poFileRead();

    const void *getDataBuffer();
    size_t getDataBufferLength();
    void writeBufferToFile(const char *filename);

    const git_oid *gitBlobHash();
    const git_oid *calculateTpHash();
    std::vector<MessageGroup *> readMessages();
    const FileDateTime &date();
    const FileDateTime &potDate();
    std::string author() const;
    void setAuthor(std::string author);

    const std::vector<int> &getMinIds();
    int getFirstId();
    int getIdCount();

    void writeToFile(const char *dest_filename, bool force_write);
    void writeToFile();

    // Does the same as "msgmerge"
    void copyTranslationsFrom(TranslationContent *from);

private:
    void clear();

    // Helpers for poFileRead()
    po_file_t poreadFile();
    po_file_t poreadGit();
    po_file_t poreadBuffer();

    // Helpers for getDataBuffer() and getDataBufferLength()
    void loadToBuffer();
    void loadToBufferFile();
    void loadToBufferGit();

    // for calculateTpHash
    std::string dumpPoFileTemplate();

    void readMessagesInternal(std::vector<MessageGroup *> &dest, bool &destInit);

    void initFirstIdPair();

    void assertOk();

    MessageGroup *findMessageGroupByOrig(const MessageGroup *msg);

private:
    char *m_filename;
    char *m_displayFilename;

    GitLoaderBase *m_gitLoader;
    git_oid *m_oid;
    git_oid *m_tphash;

    const void *m_buffer;
    size_t m_bufferLen;

    int m_type;

    std::vector<int> m_minIds;
    bool m_minIdsInit; // "true" if m_minIds is initialized

    std::vector<MessageGroup *> m_messagesNormal;
    bool m_messagesNormalInit;

    FileDateTime m_date;
    FileDateTime m_potDate;
    std::string m_author;

    int m_firstId;
    int m_idCount;

    enum
    {
        TYPE_UNKNOWN = 0,
        TYPE_FILE = 1,
        TYPE_GIT = 2,
        TYPE_BUFFER = 3,
        TYPE_DYNAMIC = 4 // the content of .po file can't be re-read from any other sources, only "m_messagesNormal" is valid
    };
};

std::vector<MessageGroup *> read_po_file_messages(const char *filename, bool loadObsolete);

#endif // TRANSLATIONCONTENT_H

