
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

//------------- TrDbOffsets ---------------
class TrDbOffsets : public MappedFile
{
public:
	TrDbOffsets(const char *filename);

	void writeOffset(int msg_id, trdb_offset offset);
	trdb_offset readOffset(int msg_id) const;
};

//------------- TrDbStrings ---------------
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

//------------- TrDb ---------------
class TrDb
{
public:
	TrDb(const char *db_dir);
	~TrDb();

	trdb_offset currentOffset() const;
	void appendData(const void *data, size_t len);
	template<class T> void appendData(const T &data);
	void appendString(const char *str);

	void addIdMessage(int msg_id, CommitInfo *commit_info, Message *message);
	std::vector<Message *> getMessages(int msg_id);

protected:
	trdb_offset writeMessage(CommitInfo *commit_info, Message *message);
	trdb_offset listNext(trdb_offset offset) const;
	trdb_offset listLast(trdb_offset offset) const;
	void listAppendMessage(trdb_offset offset, CommitInfo *commit_info, Message *message);

private:
	TrDbOffsets *m_offsets;
	TrDbStrings *m_strings;
};

//------------- CommitInfo (implementation) ---------------
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

void CommitInfo::write(TrDb *tr_db)
{
	m_dbOffset = tr_db->currentOffset();
	tr_db->appendData(m_date);
	tr_db->appendString(m_commitId);
	tr_db->appendString(m_author);

	m_writtenToDb = true;
}

trdb_offset CommitInfo::dbOffset() const
{
	return m_dbOffset;
}

//------------- TrDbOffsets (implementation) ---------------

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
	if (sizeof(trdb_offset) * (msg_id + 1) > fileLength())
		return 0;
	else
		return *(const trdb_offset *)ptrConst(sizeof(trdb_offset) * msg_id, sizeof(trdb_offset));
}

//------------- TrDbStrings (implementation) ---------------

TrDbStrings::TrDbStrings(const char *filename):
	MappedFile(filename)
{
	// Always store at least the actual length
	if (refActualLength() == 0)
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

	refActualLength() += len;
}

//------------- TrDb (implementation) ---------------

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

template<class T> void TrDb::appendData(const T &data)
{
	appendData(&data, sizeof(T));
}

void TrDb::appendString(const char *str)
{
	appendData(str, strlen(str) + 1);
}

// Use this instead of TrDb::listAppendMessage only when initializing a new list of messages
trdb_offset TrDb::writeMessage(CommitInfo *commit_info, Message *message)
{
	trdb_offset offset = currentOffset();

	trdb_offset next_offset = 0; // yes, it's just zero (i.e. last message in the list)
	trdb_offset commit_info_offset = commit_info->dbOffset();
	char fuzzy = message->isFuzzy() ? 1 : 0;
	char num_plurals = message->isPlural() ? message->numPlurals() : 0;

	appendData(next_offset);
	appendData(commit_info_offset);
	appendData(fuzzy);
	appendData(num_plurals);

	appendString(message->msgcomments());
	printf("numPlurals = %d\n", message->numPlurals());
	for (int i = 0; i < message->numPlurals(); i ++)
		appendString(message->msgstr(i));

	return offset;
}

// offset -- offset of the message
//
// Returns the offset of the next message.
trdb_offset TrDb::listNext(trdb_offset offset) const
{
	return *(trdb_offset *)m_strings->ptrConst((size_t)offset, sizeof(trdb_offset));
}

trdb_offset TrDb::listLast(trdb_offset offset) const
{
	while (listNext(offset) != 0)
		offset = listNext(offset);

	return offset;
}

// Write the message to strings.mmapdb and link it to the
// message list that begins (or continues, i.e. the offset
// may point to a message in the middle of the list) at the given offset.
void TrDb::listAppendMessage(trdb_offset offset, CommitInfo *commit_info, Message *message)
{
	trdb_offset offset_new = writeMessage(commit_info, message);
	trdb_offset offset_last = listLast(offset);

	// link the new message to the last message
	*(trdb_offset *)m_strings->ptr((size_t)offset_last, sizeof(trdb_offset)) = offset_new;
}

void TrDb::addIdMessage(int msg_id, CommitInfo *commit_info, Message *message)
{
	trdb_offset offset = m_offsets->readOffset(msg_id);
	if (offset == 0) // a new list needs to be created
	{
		offset = writeMessage(commit_info, message);
		m_offsets->writeOffset(msg_id, offset);
	}
	else
	{
		listAppendMessage(offset, commit_info, message);
	}
}

std::vector<Message *> TrDb::getMessages(int msg_id)
{
	std::vector<Message *> res;
	for (trdb_offset offset = m_offsets->readOffset(msg_id); offset != 0; offset = listNext(offset))
	{
		bool fuzzy = (*(const char *)m_strings->ptrConst(
			offset + 2 * sizeof(trdb_offset),
			sizeof(char)) == 1);
		int n_plurals = (int)*(const char *)m_strings->ptrConst(
			offset + 2 * sizeof(trdb_offset) + sizeof(char),
			sizeof(char));

		// We cannot know the lengths of strings before reading them, so just passing length=1 to ptr()
		const char *str = (const char *)m_strings->ptr(offset + 2 * sizeof(trdb_offset) + 2 * sizeof(char), 1);

		Message *msg = new Message(fuzzy, n_plurals, str); // str = msgcomment
		for (int i = 0; i < msg->numPlurals(); i ++)
		{
			str += strlen(str) + 1;
			msg->setMsgstr(i, str);
		}

		res.push_back(msg);
	}

	return res;
}

//-----------------------------------------

int main(int argc, char *argv[])
{
	TrDb *tr_db = new TrDb(".");

	CommitInfo *commit_info = new CommitInfo("SVN r123", "me", (time_t)-1);
	commit_info->write(tr_db);

	Message *message;

	message = new Message(true, "translators'-comments-for-message", "translation-of-message");
	tr_db->addIdMessage(100, commit_info, message);

	message = new Message(false, "translators'-comments-for-message", "translation-of-message", 1);
	tr_db->addIdMessage(100, commit_info, message);

	std::vector<Message *> list = tr_db->getMessages(100);
	for (size_t i = 0; i < list.size(); i ++)
	{
		printf("%s\n", list[i]->msgcomments());
		printf("%s\n", list[i]->msgstr(0));
		printf("%s\n", list[i]->isFuzzy() ? "fuzzy" : "not fuzzy");
	}

	return 0;
}

