// WindowsFileSelectorUI.hpp
#ifdef _WIN32
#pragma once
#include "FileSelector.hpp"
#include "IFileSelectorUI.hpp"
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

class WindowsFileSelectorUI : public IFileSelectorUI {
public:
    WindowsFileSelectorUI(const fs::path &start, const std::vector<std::string> &exts);
    std::vector<fs::path> selectMultipleFile() override;

private:
    fs::path startPath;
    std::vector<std::string> extensions;
};
#endif // _WIN32
