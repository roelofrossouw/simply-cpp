#include "core.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <random>
#include <stdexcept>

namespace sc {
    std::string file_get_contents(const std::string &filename) {
        std::stringstream stream;
        std::ifstream input_file(filename.c_str());
        input_file >> stream.rdbuf();
        return stream.str();
    }

    std::string basename(const std::string &filename) {
        const std::filesystem::path app_name(filename);
        return app_name.filename();
    }

    void file_put_contents(const std::string &filename, const std::string &content, const std::ios_base::openmode mode) {
        std::ofstream output_file(filename, mode);
        output_file << content;
        output_file.close();
    }

    double rand(double minval, double maxval) {
        std::random_device r;
        return static_cast<double>(r()) / RAND_MAX / 2 * (maxval - minval) + minval;
    }

    std::string getenv(const std::string &variable, const std::string &fallback) {
        const char *value = std::getenv(variable.c_str());
        return value && *value ? value : fallback;
    }

    std::vector<std::string> explode(const std::string &text, const std::string &separator) {
        if (separator.empty()) throw std::invalid_argument{"explode: separator cannot be empty"};
        std::vector<std::string> items;
        std::string::size_type start = 0;
        for (auto found = text.find(separator); found != std::string::npos; found = text.find(separator, start)) {
            items.emplace_back(text.substr(start, found - start));
            start = found + separator.size();
        }
        items.emplace_back(text.substr(start));
        return items;
    }
}
