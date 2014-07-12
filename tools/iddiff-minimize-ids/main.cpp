#include <gtpo/iddiff.h>

#include <cassert>

int main(int argc, char *argv[])
{
    assert(argc == 2 || argc == 3); // 1 or 2 arguments
    const char *src_path = argv[1];
    const char *dest_path = (argc == 3) ? argv[2] : src_path;

    Iddiffer *src_diff = new Iddiffer();
    if (!src_diff->loadIddiff(src_path)) // src_path does not exist
        return 1;

    src_diff->minimizeIds();
    src_diff->writeToFile(dest_path);

    return 0;
}

