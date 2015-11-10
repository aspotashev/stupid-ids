#ifndef FILECONTENTBASE_H
#define FILECONTENTBASE_H

#include <gtpo/gitoid.h>

#include <gettext-po.h>

#include <string>

class FileContentBase
{
public:
    /**
    * \brief Constructs a TranslationContent from a file.
    *
    * \param filename File name.
    */
    FileContentBase(const std::string& displayFilename = std::string());

    virtual ~FileContentBase();

    // Caller should run 'po_file_free'
    virtual po_file_t poFileRead() const = 0;

    const void *getDataBuffer();
    size_t getDataBufferLength();
    void writeBufferToFile(const std::string& filename);

    const git_oid *gitBlobHash();

private:
    virtual git_oid *gitBlobHashImpl() const = 0;

    // Helper for getDataBuffer() and getDataBufferLength()
    virtual void loadToBuffer() = 0;

protected:
    const void* m_buffer;
    size_t m_bufferLen;

    // Cached Git blob hash
    git_oid* m_oid;
};

#endif // FILECONTENTBASE_H
