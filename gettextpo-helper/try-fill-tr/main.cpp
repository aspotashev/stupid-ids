
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

typedef long long int trdb_offset;

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

//-----------------------------------------

int main(int argc, char *argv[])
{
	TrDbOffsets *db_offsets = new TrDbOffsets("./offsets.mmapdb");
	TrDbStrings *db_strings = new TrDbStrings("./strings.mmapdb");

	return 0;
}

