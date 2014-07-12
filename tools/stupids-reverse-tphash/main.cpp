#include <gtpo/oidmapcache.h>

#include <iostream>
#include <cassert>

// Input: tp_hash
// Output: list of OIDs
int main(int argc, char *argv[])
{
	assert(argc == 2); // 1 argument

	GitOid tp_hash(argv[1]);
	std::vector<GitOid> res = TphashCache.reverseGetValues(tp_hash.oid());

	for (size_t i = 0; i < res.size(); i ++)
		std::cout << res[i].toString() << std::endl;

	return 0;
}

