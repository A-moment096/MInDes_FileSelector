#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class IFileSelectorUI {
public:
    virtual ~IFileSelectorUI() = default;
    virtual std::vector<fs::path> selectMultipleFile() = 0;
    virtual fs::path selectSingleFile() = 0;
};
