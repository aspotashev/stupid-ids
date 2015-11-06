#include "filecontentfs.h"

#include <gtpo/gettextpo-helper.h>

#include <git2/odb.h>

#include <cstdio>
#include <cassert>
#include <stdexcept>

FileContentFs::FileContentFs(const std::string& filename)
    : FileContentBase(filename)
    , m_filename(filename)
{
}

FileContentFs::~FileContentFs()
{
}

po_file_t FileContentFs::poFileRead()
{
    assert(!m_filename.empty());

    return po_file_read(m_filename.c_str());
}

std::string FileContentFs::filename() const
{
    return m_filename;
}

git_oid* FileContentFs::gitBlobHashImpl() const
{
    FILE *f = fopen(m_filename.c_str(), "r");
    if (!f) {
        printf("Could not open file %s\n", m_filename.c_str());

        return nullptr;
    }

    git_oid *oid = new git_oid;

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    if (file_size < 0)
        throw std::runtime_error("ftell() failed");
    rewind(f);

    char *temp_buffer = new char[file_size];
    assert(fread(temp_buffer, 1, file_size, f) == static_cast<size_t>(file_size));
    fclose(f);

    assert(git_odb_hash(oid, temp_buffer, file_size, GIT_OBJ_BLOB) == 0);
    delete [] temp_buffer;

    return oid;
}

void FileContentFs::loadToBuffer()
{
    if (m_buffer) {
        return;
    }

    assert(0); // not used for now, someone will write this later
}
