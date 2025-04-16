// FileSelector.hpp
#pragma once
#include "IFileSelectorUI.hpp"
#include <memory>
#include <string>
#include <vector>

class FileSelector {
public:
    FileSelector(const std::string &start, const std::vector<std::string> &exts);
    std::vector<std::string> run();

private:
    std::unique_ptr<IFileSelectorUI> uiImpl;
};