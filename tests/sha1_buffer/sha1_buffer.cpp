#include <gtpo/gettextpo-helper.h>
#include <gtpo/detectorbase.h>
#include <gtpo/block-sha1/sha1.h>

#include <iostream>

int main()
{
	const char buffer[] = "Hello, world!";
	size_t len = sizeof(buffer);

	git_oid oid;
	sha1_buffer(&oid, buffer, len);

	GitOid a(&oid);
	GitOid b("2d5ec68b0d061c75db234c2fd10bde63528fa0a1");
	if (a != b)
	{
		std::cout << "a = " << a.toString() << std::endl;
		std::cout << "b = " << b.toString() << std::endl;
		return 1; // fail
	}

	return 0; // ok
}
