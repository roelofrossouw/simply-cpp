#pragma once
#include <ios>
#include <string>
#include <vector>

// Small PHP-style helpers.
namespace sc {
    // The whole file; empty when it can't be read.
    std::string file_get_contents(const std::string &filename);

    // The file name without its directories: "/var/log/syslog.1" gives "syslog.1".
    std::string basename(const std::string &filename);

    // Writes content to the file, replacing it; std::ios::app appends instead.
    void file_put_contents(const std::string &filename, const std::string &content, std::ios_base::openmode mode = std::ios_base::binary);

    // A random number from minval to maxval, from std::random_device.
    double rand(double minval = 0, double maxval = 1);

    // The value of an environment variable, or fallback when it is unset or empty.
    // Takes a std::string so a plain getenv("NAME") still means the C function.
    std::string getenv(const std::string &variable, const std::string &fallback = {});

    // Splits text on every occurrence of separator, like PHP's explode(): empty items are
    // kept ("a;;b" gives "a", "", "b") and empty text gives one empty item. Throws
    // std::invalid_argument when separator is empty.
    std::vector<std::string> explode(const std::string &text, const std::string &separator = ";");
}
