#include <gtpo/iddiff.h>

#include <assert.h>

// TODO: merge into "tools/iddiff-merge" (as optional mode)
int toolIddiffMergeTrcomm(int argc, char *argv[])
{
    const std::vector<std::string>& args = parseArgs(argc, argv);

    assert(args.size() == 2); // 2 arguments
    std::string src_path = args[0];
    std::string merged_path = args[1];

    Iddiff* src_diff = new Iddiff();
    if (!src_diff->loadIddiff(src_path.c_str())) // src_path does not exist
        return 1;

    Iddiff* merged_diff = new Iddiff();
    if (merged_diff->loadIddiff(merged_path.c_str())) {
        // there was a file
        merged_diff->mergeTrComments(src_diff);
        merged_diff->writeToFile(merged_path.c_str());
    }
    else {
        // merged_diff did not exist
        src_diff->writeToFile(merged_path.c_str());
    }

    return 0;
}
