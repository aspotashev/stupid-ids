#include <gtest/gtest.h>

#include <gtpo/block-sha1/sha1.h>
#include <gtpo/gitoid.h>

// Testing here:
// sha1_buffer()
TEST(Sha1Buffer, Sha1Buffer) {
    const char buffer[] = "Hello, world!";
    size_t len = sizeof(buffer);

    git_oid oid;
    sha1_buffer(&oid, buffer, len);

    EXPECT_EQ("2d5ec68b0d061c75db234c2fd10bde63528fa0a1", GitOid(&oid).toString());
}
