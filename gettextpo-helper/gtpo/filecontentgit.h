#ifndef FILECONTENTGIT_H
#define FILECONTENTGIT_H

#include "filecontentbase.h"

#include <gtpo/gitoid.h>

#include <gettext-po.h>

#include <string>

class GitLoaderBase;

class FileContentGit : public FileContentBase
{
public:
    /**
    * \brief Constructs a TranslationContent from a Git blob identified by its OID.
    *
    * \param git_loader Git repositories list used for searching the blob by OID.
    * \param oid OID of the blob.
    */
    FileContentGit(GitLoaderBase *git_loader, const git_oid *oid);

    ~FileContentGit();

    // Caller should run 'po_file_free'
    virtual po_file_t poFileRead() const;

private:
    virtual git_oid *gitBlobHashImpl() const;
    virtual void loadToBuffer();

private:
    GitLoaderBase* m_gitLoader;
};

#endif // FILECONTENTGIT_H
