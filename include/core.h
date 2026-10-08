#pragma once
#include <ios>
#include <string>
#include <vector>

// Some common functions simplifying STL
namespace sc {
    std::string file_get_contents(const std::string &filename);

    std::string basename(const std::string &filename);

    void file_put_contents(const std::string &filename, const std::string &content, std::ios_base::openmode mode = std::ios_base::binary);

    double rand(double minval = 0, double maxval = 1);

    // The value of an environment variable, or fallback when it is unset or empty.
    // Takes a std::string so a plain getenv("NAME") still means the C function.
    std::string getenv(const std::string &variable, const std::string &fallback = {});

    // Splits text on every occurrence of separator, like PHP's explode(): empty items are
    // kept ("a;;b" gives "a", "", "b") and empty text gives one empty item. Throws
    // std::invalid_argument when separator is empty.
    std::vector<std::string> explode(const std::string &text, const std::string &separator = ";");
}
