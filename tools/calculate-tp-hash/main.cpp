#include <gtpo/gettextpo-helper.h>

#include <iostream>

void toolCalculateTpHash(std::vector<std::string> args)
{
    std::string filename;
    if (args.size() == 0)
        filename = "-";
    else if (args.size() == 1)
        filename = args[0];
    else
        assert(0); // too many arguments

    std::cout << calculate_tp_hash(filename.c_str());
}

int main(int argc, char *argv[])
{
    toolCalculateTpHash(parseArgs(argc, argv));
    return 0;
}
