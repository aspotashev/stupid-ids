#ifndef MAPPEDFILE_H
#define MAPPEDFILE_H

#include <stdlib.h>
#include <vector>

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
	int getPlainMinId(int msg_id) const;

	// put globally minimum IDs into 'min_id'
	void normalizeDatabase();

	std::vector<int> getMinIdArray(int first_msg_id, int num);
};

#endif // MAPPEDFILE_H

