#include "filecontentbuffer.h"

#include <gtpo/gettextpo-helper.h>

#include <git2/odb.h>

#include <cstdio>
#include <cassert>
#include <stdexcept>

FileContentBuffer::FileContentBuffer(const void* buffer, size_t len)
    : FileContentBase()
{
    m_buffer = buffer; // take ownership of the buffer
    m_bufferLen = len;
}

FileContentBuffer::~FileContentBuffer()
{
}

po_file_t FileContentBuffer::poFileRead()
{
    assert(m_buffer);

    return po_buffer_read((const char *)m_buffer, m_bufferLen);
}

git_oid* FileContentBuffer::gitBlobHashImpl() const
{
    git_oid *oid = new git_oid;

    assert(git_odb_hash(oid, m_buffer, m_bufferLen, GIT_OBJ_BLOB) == 0);

    return m_oid;
}

void FileContentBuffer::loadToBuffer()
{
    // data should already be buffered (by definition of FileContentBuffer)
    assert(m_buffer);
}
