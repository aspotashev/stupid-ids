#include "filecontentbase.h"

#include <cstdio>
#include <cassert>

FileContentBase::FileContentBase(const std::string& displayFilename)
    : m_buffer(nullptr)
    , m_bufferLen(0)
    , m_oid(nullptr)
{
}

FileContentBase::~FileContentBase()
{
    if (m_buffer) {
        delete [] (char *)m_buffer;
    }

    if (m_oid) {
        delete m_oid;
    }
}

const void *FileContentBase::getDataBuffer()
{
    if (!m_buffer) {
        // if the data has not been buffered yet
        loadToBuffer();
    }

    return m_buffer;
}

size_t FileContentBase::getDataBufferLength()
{
    if (!m_buffer) {
        // if the data has not been buffered yet
        loadToBuffer();
    }

    return m_bufferLen;
}

void FileContentBase::writeBufferToFile(const std::string& filename)
{
    loadToBuffer();

    FILE *f = fopen(filename.c_str(), "w");
    assert(f);
    assert(fwrite(m_buffer, 1, m_bufferLen, f) == m_bufferLen);
    fclose(f);
}

const git_oid* FileContentBase::gitBlobHash()
{
    if (!m_oid) {
        m_oid = gitBlobHashImpl();
    }

    return m_oid;
}
