#include "tools.h"

#include <map>
#include <iostream>

std::map<std::string, int(*)(int argc, char *argv[])> tools = {
//     {"find-best-matching-translated", toolFindBestMatchingTranslated},
    {"iddiff", toolIddiff},
    {"iddiff-git", toolIddiffGit},
    {"iddiff-merge", toolIddiffMerge},
    {"iddiff-merge-trcomm", toolIddiffMergeTrcomm},
    {"iddiff-minimize-ids", toolIddiffMinimizeIds},
//     {"iddiff-repo", toolIddiffRepo},
    {"iddiff-review-format-mailbox", toolIddiffReviewFormatMailbox},
    {"idpatch", toolIdpatch},
    {"lokalize-reviewfile", toolLokalizeReviewfile},
    {"lokalize-review-iddiff", toolLokalizeReviewIddiff},
    {"rerere-trusted-filter", toolRerereTrustedFilter},
    {"reverse-tphash", toolReverseTpHash},
    {"try-fill-tr", toolTryFillTranslations},

//     {"iddiff-test", },
};

void printHelp()
{
    std::cerr << std::endl;
    std::cerr << "Available commands are:" << std::endl;
    for (auto tool : tools)
        std::cerr << "    " << tool.first << std::endl;
}

int main(int argc, char *argv[])
{
    if (argc <= 1) {
        std::cerr << "Command not specified, exiting." << std::endl;
        printHelp();
        return 1;
    }

    std::string cmd(argv[1]);
    auto iter = tools.find(cmd);
    if (iter == tools.end()) {
        std::cerr << "Command \"" << cmd << "\" is not defined, exiting." << std::endl;
        printHelp();
        return 1;
    }

    return iter->second(argc - 1, argv + 1);
}
