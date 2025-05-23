#include "src/FileSelector.hpp"
#include <filesystem>
#include <fmt/core.h>
#include <string>
#include <vector>

namespace fs = std::filesystem;

int main() {
    std::vector<fs::path> files;
    try {
        FileSelector selector("~", {"mindes", "txt"});
        files = selector.selectMultipleFile();
    } catch (const std::exception &e) {
        fmt::print("Error: {}\n", e.what());
        return 1;
    }

    fs::path file;
    try {
        FileSelector singleSelector{"~", {}};
        file = singleSelector.selectSingleFile();
    } catch (const std::exception &e) {
        fmt::print("Error: {}\n", e.what());
        return 1;
    }

    for (auto &f : files) {
        fmt::print("{}\n", f.string());
    }
    fmt::print("Single File selection:\n{}\n", file.string());
    return 0;
}
