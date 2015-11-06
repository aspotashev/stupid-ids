#include "filecontentgit.h"
#include "gitloader.h"

#include <gtpo/gettextpo-helper.h>

#include <git2/odb.h>

#include <cstdio>
#include <cassert>
#include <cstring>
#include <stdexcept>

FileContentGit::FileContentGit(GitLoaderBase *git_loader, const git_oid *oid)
    : FileContentBase()
    , m_gitLoader(git_loader)
{
    m_oid = new git_oid;
    git_oid_cpy(m_oid, oid);
}

FileContentGit::~FileContentGit()
{
}

// TODO: use m_buffer if it is initialized
po_file_t FileContentGit::poFileRead()
{
    assert(m_gitLoader);
    assert(m_oid);

    git_blob *blob = m_gitLoader->blobLookup(GitOid(m_oid));
    if (!blob)
        return NULL;

    const void *rawcontent = git_blob_rawcontent(blob);
    int rawsize = git_blob_rawsize(blob);
    po_file_t file = po_buffer_read((const char *)rawcontent, (size_t)rawsize);

    git_blob_free(blob);

    return file;
}

git_oid* FileContentGit::gitBlobHashImpl() const
{
    // If type is TYPE_GIT, then m_oid must be already initialized
    return m_oid;
}

void FileContentGit::loadToBuffer()
{
    if (m_buffer) {
        return;
    }

    assert(m_gitLoader);
    assert(m_oid);

    git_blob *blob = m_gitLoader->blobLookup(m_oid);
    if (!blob)
        return;

    const void *rawcontent = git_blob_rawcontent(blob);
    int rawsize = git_blob_rawsize(blob);
    assert(rawsize > 0);

    char *rawcontent_copy = new char[rawsize];
    assert(rawcontent_copy);
    memcpy(rawcontent_copy, rawcontent, (size_t)rawsize);

    git_blob_free(blob);


    m_buffer = rawcontent_copy;
    m_bufferLen = (size_t)rawsize;
}
