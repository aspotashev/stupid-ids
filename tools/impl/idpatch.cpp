#include <gtpo/translation-collector.h>
#include <gtpo/iddiff.h>
#include <gtpo/config.h>

#include <iostream>
#include <cassert>
#include <cstdio>
#include <cstring>

int toolIdpatch(int argc, char *argv[])
{
    assert(argc == 3); // 2 arguments (mode; path to .iddiff)

    bool comments = false;
    if (!strcmp(argv[1], "messages"))
        comments = false;
    else if (!strcmp(argv[1], "comments"))
        comments = true;
    else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        return 1;
    }

    Iddiff* differ = new Iddiff();
    differ->loadIddiff(argv[1]);
    differ->minimizeIds();

    std::cout << differ->generateIddiffText();

    // Apply patch
    StupIdTranslationCollector collector;
    collector.insertPoDirOrTemplate(
        expand_path(StupidsConf("apply.templates_path.trunk")).c_str(),
        expand_path(StupidsConf("apply.translations_path.trunk")).c_str());
    collector.insertPoDirOrTemplate(
        expand_path(StupidsConf("apply.templates_path.stable")).c_str(),
        expand_path(StupidsConf("apply.translations_path.stable")).c_str());

    if (comments)
        differ->applyIddiffComments(&collector);
    else
        differ->applyIddiff(&collector);

    return 0;
}
