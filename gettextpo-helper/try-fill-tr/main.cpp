
// http://www.gnu.org/software/gettext/manual/gettext.html#libgettextpo
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <vector>
#include <map>

#include "../include/gettextpo-helper.h"
#include "../ruby-ext/mappedfile.h"

typedef long long int trdb_offset; // 64-bit

char *xstrdup(const char *str)
{
	size_t len = strlen(str);
	char *dup = new char [len + 1];
	strcpy(dup, str);

	return dup;
}

//------------- CommitInfo ---------------
class TrDb;

class CommitInfo
{
public:
	CommitInfo(const char *commit_id, const char *author, time_t date);
	~CommitInfo();

	void write(TrDb *tr_db);
	trdb_offset dbOffset() const;

private:
	char *m_commitId;
	char *m_author;
	time_t m_date;

	bool m_writtenToDb;
	trdb_offset m_dbOffset;
};

CommitInfo::CommitInfo(const char *commit_id, const char *author, time_t date)
{
	assert(sizeof(m_date) == 8); // 64-bit

	m_writtenToDb = false;

	m_author = xstrdup(author);
	m_commitId = xstrdup(commit_id);

	m_date = date;
}

CommitInfo::~CommitInfo()
{
	delete [] m_author;
	delete [] m_commitId;
}

//------------- TrDbOffsets ---------------
class TrDbOffsets : public MappedFile
{
public:
	TrDbOffsets(const char *filename);

	void writeOffset(int msg_id, trdb_offset offset);
	trdb_offset readOffset(int msg_id) const;
};

TrDbOffsets::TrDbOffsets(const char *filename):
	MappedFile(filename)
{
}

void TrDbOffsets::writeOffset(int msg_id, trdb_offset offset)
{
	trdb_offset *p = (trdb_offset *)ptr(sizeof(trdb_offset) * msg_id, sizeof(trdb_offset));
	*p = offset;
}

trdb_offset TrDbOffsets::readOffset(int msg_id) const
{
	return *(const trdb_offset *)ptrConst(sizeof(trdb_offset) * msg_id, sizeof(trdb_offset));
}

//------------- TrDbOffsets ---------------
class TrDbStrings : public MappedFile
{
public:
	TrDbStrings(const char *filename);

	trdb_offset actualLength() const;
	void appendData(const void *data, size_t len);

private:
	trdb_offset &refActualLength()
	{
		return *(trdb_offset *)ptr(0, sizeof(trdb_offset));
	}
};

TrDbStrings::TrDbStrings(const char *filename):
	MappedFile(filename)
{
	// Always store at least the actual length
	refActualLength() = sizeof(trdb_offset);
}

// TODO: cache this?
trdb_offset TrDbStrings::actualLength() const
{
	return *(trdb_offset *)ptrConst(0, sizeof(trdb_offset));
}

void TrDbStrings::appendData(const void *data, size_t len)
{
	// Mapping limits the maximum size to 4 GB on 32-bit systems.
	void *data_dest = ptr((size_t)actualLength(), len);
	memcpy(data_dest, data, len);
}

//------------- TrDb ---------------
class TrDb
{
public:
	TrDb(const char *db_dir);
	~TrDb();

	trdb_offset currentOffset() const;
	void appendData(const void *data, size_t len);
	void appendString(const char *str);

private:
	TrDbOffsets *m_offsets;
	TrDbStrings *m_strings;
};

TrDb::TrDb(const char *db_dir)
{
	char *offsets_fn = new char [200]; // TODO: fix memory leak
	strcpy(offsets_fn, db_dir);
	strcat(offsets_fn, "/offsets.mmapdb");

	m_offsets = new TrDbOffsets(offsets_fn);


	char *strings_fn = new char [200]; // TODO: fix memory leak
	strcpy(strings_fn, db_dir);
	strcat(strings_fn, "/strings.mmapdb");

	m_strings = new TrDbStrings(strings_fn);
}

TrDb::~TrDb()
{
}

trdb_offset TrDb::currentOffset() const
{
	return m_strings->actualLength();
}

void TrDb::appendData(const void *data, size_t len)
{
	m_strings->appendData(data, len);
}

void TrDb::appendString(const char *str)
{
	appendData(str, strlen(str) + 1);
}

//-----------------------------------------

void CommitInfo::write(TrDb *tr_db)
{
	m_dbOffset = tr_db->currentOffset();
	tr_db->appendData(&m_date, sizeof(m_date));
	tr_db->appendString(m_author);
	tr_db->appendString(m_commitId);

	m_writtenToDb = true;
}

trdb_offset CommitInfo::dbOffset() const
{
	return m_dbOffset;
}

int main(int argc, char *argv[])
{
	TrDb *tr_db = new TrDb(".");

	CommitInfo *commit_info = new CommitInfo("SVN r123", "me", (time_t)-1);
	commit_info->write(tr_db);

	Message *message;

	message = new Message(false, "translators'-comments-for-message", "translation-of-message");

	message = new Message(false, "translators'-comments-for-message", "translation-of-message", 1);

	return 0;
}

