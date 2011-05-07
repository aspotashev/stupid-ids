
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

//------------- CommitInfo ---------------
class CommitInfo
{
public:
	CommitInfo(const char *author, time_t date);
	~CommitInfo();

private:
	char *m_author;
	time_t m_date;
};

CommitInfo::CommitInfo(const char *author, time_t date)
{
	size_t author_len = strlen(author);
	m_author = new char [author_len + 1];
	strcpy(m_author, author);

	m_date = date;
}

CommitInfo::~CommitInfo()
{
	delete [] m_author;
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

//------------- TrDb ---------------
class TrDb
{
public:
	TrDb(const char *db_dir);
	~TrDb();

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

//-----------------------------------------

int main(int argc, char *argv[])
{
	TrDb *tr_db = new TrDb(".");

	CommitInfo *commit_info = new CommitInfo("me", (time_t)-1);
	Message *message;

	message = new Message(false, "translators'-comments-for-message", "translation-of-message");

	message = new Message(false, "translators'-comments-for-message", "translation-of-message", 1);

	return 0;
}

