// WindowsFileSelectorUI.hpp
#pragma once
#include "IFileSelectorUI.hpp"
#include <string>
#include <vector>
class WindowsFileSelectorUI : public IFileSelectorUI {
public:
    WindowsFileSelectorUI(const std::string &start, const std::vector<std::string> &exts);
    std::vector<std::string> run() override;

private:
    // Store parameters as needed and encapsulate the WIN32 dialog code.
};
