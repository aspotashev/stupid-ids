#ifndef FILECONTENTFS_H
#define FILECONTENTFS_H

#include "filecontentbase.h"

#include <gtpo/gitoid.h>

#include <gettext-po.h>

#include <string>

class FileContentFs : public FileContentBase
{
public:
    /**
    * \brief Constructs a FileContentFs from a file.
    *
    * \param filename File name.
    */
    FileContentFs(const std::string& filename);

    ~FileContentFs();

    // Caller should run 'po_file_free'
    virtual po_file_t poFileRead() const;

    std::string filename() const;

private:
    virtual git_oid *gitBlobHashImpl() const;
    virtual void loadToBuffer();

private:
    std::string m_filename;
};

#endif // FILECONTENTFS_H
