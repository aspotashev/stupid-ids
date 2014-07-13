#include <gtpo/iddiff.h>

#include <cassert>

int toolIddiffMinimizeIds(int argc, char *argv[])
{
    const std::vector<std::string>& args = parseArgs(argc, argv);

    assert(args.size() == 1 || args.size() == 2); // 1 or 2 arguments

    std::string src_path = args[0];
    std::string dest_path = (args.size() == 2) ? args[1] : src_path;

    Iddiffer* src_diff = new Iddiffer();
    if (!src_diff->loadIddiff(src_path.c_str())) // src_path does not exist
        return 1;

    src_diff->minimizeIds();
    src_diff->writeToFile(dest_path.c_str());

    return 0;
}
