#include <gtpo/gettextpo-helper.h>

#include <iostream>

int toolCalculateTpHash(int argc, char *argv[])
{
    const std::vector<std::string>& args = parseArgs(argc, argv);

    std::string filename;
    if (args.size() == 0)
        filename = "-";
    else if (args.size() == 1)
        filename = args[0];
    else
        assert(0); // too many arguments

    std::cout << calculate_tp_hash(filename.c_str());

    return 0;
}
