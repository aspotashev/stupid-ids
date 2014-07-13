#include <gtpo/gettextpo-helper.h>
#include <gtpo/oidmapcache.h>

#include <iostream>
#include <cassert>

// Input: tp_hash
// Output: list of OIDs
int toolStupidsReverseTpHash(int argc, char *argv[])
{
    const std::vector<std::string>& args = parseArgs(argc, argv);

    assert(args.size() == 1); // 1 argument

    GitOid tp_hash(args[0].c_str());
    std::vector<GitOid> res = TphashCache.reverseGetValues(tp_hash.oid());

    for (size_t i = 0; i < res.size(); i ++)
        std::cout << res[i].toString() << std::endl;

    return 0;
}
