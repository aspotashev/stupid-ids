
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>

class MappedFile
{
public:
	MappedFile(const char *filename);
	~MappedFile();

	// Returns a pointer to data at the given offset in file.
	//
	// offset -- offset in file
	// len -- number of bytes that should be accessible after the given offset
	void *ptr(size_t offset, size_t len);
	const void *ptrConst(size_t offset, size_t len) const;

protected:
	size_t fileLength() const;

private:
	void detectFileSize(); // initializes m_fileLength
	void mapFile();
	void unmapFile();

private:
	char *m_filename;

	// When m_fileLength is 0, the file does not exist (or
	// it has zero length), and 'mmap' was not called.
	//
	// This value should be maintained correct even when the
	// file is not mapped.
	size_t m_fileLength;

	int fd; // 'mmap' works with a file descriptor
	void *data; // mapped virtual memory
};

MappedFile::MappedFile(const char *filename)
{
	m_filename = new char[strlen(filename) + 1];
	strcpy(m_filename, filename);

	detectFileSize(); // initializes m_fileLength
	mapFile();
}

MappedFile::~MappedFile()
{
	unmapFile();
}

void MappedFile::mapFile()
{
	if (m_fileLength > 0)
	{
		fd = open(m_filename, O_RDWR | O_CREAT, 0777);
		data = mmap(NULL, m_fileLength, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	}
}

void MappedFile::unmapFile()
{
	if (m_fileLength > 0)
	{
		munmap(data, m_fileLength);
		close(fd);
	}
}

// Used only on initialization (i.e. in constructor)
void MappedFile::detectFileSize()
{
	struct stat sb;
	int ret = stat(m_filename, &sb);
	if (ret == -1)
	{
		if (errno == ENOENT) // file does not exists
		{
			m_fileLength = 0;
			return;
		}
		else
		{
			assert(0); // unhandled error
		}
	}

	m_fileLength = sb.st_size;
}

void *MappedFile::ptr(size_t offset, size_t len)
{
	assert(len >= 1);

	// If the last required byte is not accessible
	if (offset + len > m_fileLength)
	{
		unmapFile();

		size_t newSize = ((offset + len) + 0xfffff) & (~0xfffff); // megabyte boundary
		size_t sizeDiff = newSize - m_fileLength;
		assert(sizeDiff > 0);

		// Expand file by 'sizeDiff' bytes
		FILE *f = fopen(m_filename, "a+b");
		assert(sizeof(char) == 1);
		char *temp_buf = new char[sizeDiff];
		memset(temp_buf, 0, sizeDiff);
		assert(fwrite(temp_buf, 1, sizeDiff, f) == sizeDiff);
		delete [] temp_buf;
		fclose(f);

		m_fileLength += sizeDiff;

		mapFile();
	}

	// Should now be accessible
	assert(offset + len - 1 < m_fileLength);

	return (char *)data + offset; // sizeof(char) == 1
}

const void *MappedFile::ptrConst(size_t offset, size_t len) const
{
	assert(len >= 1);

	// The last required byte should be accessible,
	// because we cannot expand the file in a const function.
	assert(offset + len - 1 < m_fileLength);

	return (const char *)data + offset; // sizeof(char) == 1
}

size_t MappedFile::fileLength() const
{
	return m_fileLength;
}

/*
int main()
{
	MappedFile *file = new MappedFile("1.mmapdb");

	int *x = (int *)file->ptr(0xcfffeff, sizeof(int));
	(*x) ++;
	printf("x = %d\n", *x);

	return 0;
}
*/


//------------ MappedFile for IdMapDb -------------

class MappedFileIdMapDb : public MappedFile
{
	struct IdMapDb_row
	{
		int min_id;
	};

protected:
	void writeRow(int msg_id, int min_id);
	IdMapDb_row *getRow(int msg_id);
	const IdMapDb_row *getRowConst(int msg_id) const;

public:
	MappedFileIdMapDb(const char *filename);

	void addRow(int msg_id, int min_id);
	int getRecursiveMinId(int msg_id) const;

	// put globally minimum IDs into 'min_id'
	void normalizeDatabase();

	std::vector<int> getMinIdArray(int first_msg_id, int num);
};

MappedFileIdMapDb::MappedFileIdMapDb(const char *filename):
	MappedFile(filename)
{
}

MappedFileIdMapDb::IdMapDb_row *MappedFileIdMapDb::getRow(int msg_id)
{
	return (MappedFileIdMapDb::IdMapDb_row *)ptr(sizeof(IdMapDb_row) * msg_id, sizeof(IdMapDb_row));
}

const MappedFileIdMapDb::IdMapDb_row *MappedFileIdMapDb::getRowConst(int msg_id) const
{
	const static IdMapDb_row zeroRow = {0};

	if (sizeof(IdMapDb_row) * msg_id >= fileLength())
		return &zeroRow;
	else
		return (const MappedFileIdMapDb::IdMapDb_row *)ptrConst(sizeof(IdMapDb_row) * msg_id, sizeof(IdMapDb_row));
}

// Find the minimum ID from those that are collapsed with 'msg_id'.
int MappedFileIdMapDb::getRecursiveMinId(int msg_id) const
{
	int prev_id;
	while (msg_id = getRowConst(prev_id = msg_id)->min_id)
		assert(msg_id < prev_id); // avoiding infinite loop

	return prev_id;
}

// 'Collapse' two IDs
void MappedFileIdMapDb::addRow(int msg_id, int min_id)
{
	assert(msg_id != min_id);

	int global_min = std::min(getRecursiveMinId(msg_id), getRecursiveMinId(min_id));
	if (msg_id != global_min) // This means that msg_id > global_min
		writeRow(msg_id, global_min);
	if (min_id != global_min) // This means that min_id > global_min
		writeRow(min_id, global_min);
}

// Simply write the row
void MappedFileIdMapDb::writeRow(int msg_id, int min_id)
{
	// Too large IDs may be unsafe, because they
	// take a lot of disk and virtual memory.
	assert(msg_id >= 100 && msg_id < 100*1000*1000);

	IdMapDb_row *row = getRow(msg_id);
	row->min_id = min_id;
}

// put globally minimum IDs into 'min_id'
void MappedFileIdMapDb::normalizeDatabase()
{
	// This is not the exact number of rows
	int row_count = (int)(fileLength() / sizeof(IdMapDb_row));

	int changed_count = 0;
	for (int msg_id = 100; msg_id < row_count; msg_id ++)
	{
		IdMapDb_row *row = getRow(msg_id);

		int recurs_min_id = getRecursiveMinId(msg_id);
		if (row->min_id != 0 && row->min_id != recurs_min_id)
		{
			row->min_id = recurs_min_id;
			changed_count ++;
		}
	}

	printf("normalizeDatabase: %d changes\n", changed_count);
}

std::vector<int> MappedFileIdMapDb::getMinIdArray(int first_msg_id, int num)
{
	assert(num > 0);
	assert(first_msg_id >= 100);

	std::vector<int> res(num);
	for (int i = 0; i < num; i ++)
		res[i] = getRecursiveMinId(first_msg_id + i);

	return res;
}

