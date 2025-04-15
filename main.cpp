#include "src/FileSelector.hpp"
// #include "file_selector.hpp"
#include <fmt/core.h>

int main() {
    std::vector<std::string> files;
    try {
        FileSelector selector(".", {});
        files = selector.run();
    } catch (const std::exception &e) {
        fmt::print("Error: {}\n", e.what());
        return 1;
    }

    for (auto &f : files) {
        fmt::print("{}\n", f);
    }
    return 0;
}
