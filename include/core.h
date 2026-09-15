#pragma once
#include <ios>
#include <string>

// Some common functions simplifying STL
namespace sc {
    std::string file_get_contents(const std::string &filename);

    std::string basename(const std::string &filename);

    void file_put_contents(const std::string &filename, const std::string &content, std::ios_base::openmode mode = std::ios_base::binary);

    double rand(double minval = 0, double maxval = 1);
}
