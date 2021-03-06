#include <gtpo/translationcontent.h>
#include <gtpo/filecontentfs.h>
#include <gtpo/iddiff.h>

#include <iostream>
#include <cassert>

int toolIddiff(int argc, char *argv[])
{
    const std::vector<std::string>& args = parseArgs(argc, argv);

    Iddiff* diff = new Iddiff();
    Iddiff* diff_trcomments = new Iddiff();
    if (args.size() == 1) {
        TranslationContent* content = new TranslationContent(new FileContentFs(args[0]));
        diff->diffAgainstEmpty(content);
        diff_trcomments->diffTrCommentsAgainstEmpty(content);
    }
    else if (args.size() == 2) {
        TranslationContent* content_a = new TranslationContent(new FileContentFs(args[0]));
        TranslationContent* content_b = new TranslationContent(new FileContentFs(args[1]));
        diff->diffFiles(content_a, content_b);
        diff_trcomments->diffTrCommentsFiles(content_a, content_b);
    }
    else {
        std::cerr << "Invalid number of arguments (" << args.size() << ")" << std::endl;
        abort();
    }

    std::cout << diff->generateIddiffText();

    // TODO: add an option to choose path to .iddiff-trcomm or disable dumping of comments
    diff_trcomments->writeToFile("/tmp/11235.iddiff-tr-comments");

    return 0;
}
