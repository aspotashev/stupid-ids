#ifndef FILECONTENTBUFFER_H
#define FILECONTENTBUFFER_H

#include "filecontentbase.h"

#include <gtpo/gitoid.h>

#include <gettext-po.h>

#include <string>

class FileContentBuffer : public FileContentBase
{
public:
    /**
    * \brief Constructs a TranslationContent using data from a buffer.
    *
    * \param buffer Buffer.
    * \param len Size of the buffer.
    */
    FileContentBuffer(const void* buffer, size_t len);

    ~FileContentBuffer();

    // Caller should run 'po_file_free'
    virtual po_file_t poFileRead();

private:
    virtual git_oid *gitBlobHashImpl() const;
    virtual void loadToBuffer();
};

#endif // FILECONTENTBUFFER_H
