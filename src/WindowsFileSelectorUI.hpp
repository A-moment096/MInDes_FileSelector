// WindowsFileSelectorUI.hpp
#ifdef _WIN32
#pragma once
#include "FileSelector.hpp"
#include "IFileSelectorUI.hpp"
#include <string>
#include <vector>

class WindowsFileSelectorUI : public IFileSelectorUI {
public:
    WindowsFileSelectorUI(const std::string &start, const std::vector<std::string> &exts);
    std::vector<std::string> run() override;

private:
    std::string startPath;
    std::vector<std::string> extensions;
};
#endif // _WIN32
