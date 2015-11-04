#include "common.h"

#include <sstream>
#include <fstream>
#include <stdexcept>

std::string read_file_contents(std::string filename)
{
    std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);
    if (!in) {
        std::stringstream ss;
        ss << "Failed to open file " << filename;
        throw std::runtime_error(ss.str());
    }

    std::ostringstream ss;
    ss << in.rdbuf();
    in.close();
    return ss.str();
}
