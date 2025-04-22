#ifdef __unix__
#pragma once
#include "IFileSelectorUI.hpp"
#include <string>
#include <vector>

#include <filesystem>
namespace fs = std::filesystem;

class UnixFileSelectorUI : public IFileSelectorUI {
public:
    UnixFileSelectorUI(const fs::path &start, const std::vector<std::string> &exts);
    std::vector<fs::path> run() override;

private:
    fs::path startPath;
    std::vector<std::string> extensions;
};
#endif // __unix__
