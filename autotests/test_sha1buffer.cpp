#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <gtpo/block-sha1/sha1.h>
#include <gtpo/gitoid.h>

BOOST_AUTO_TEST_SUITE(Sha1Buffer)

// Testing here:
// sha1_buffer()
BOOST_AUTO_TEST_CASE(Sha1Buffer)
{
    const char buffer[] = "Hello, world!";
    size_t len = sizeof(buffer);

    git_oid oid;
    sha1_buffer(&oid, buffer, len);

    BOOST_CHECK_EQUAL("2d5ec68b0d061c75db234c2fd10bde63528fa0a1", GitOid(&oid).toString());
}

BOOST_AUTO_TEST_SUITE_END()
