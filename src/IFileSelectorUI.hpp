#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

class IFileSelectorUI {
public:
    virtual ~IFileSelectorUI() = default;
    virtual std::vector<fs::path> run() = 0;
};
