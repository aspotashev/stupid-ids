#ifndef SHA1_H
#define SHA1_H

#include <git2/oid.h>

void sha1_buffer(git_oid *oid, const void *buffer, size_t length);

#endif // SHA1_H
