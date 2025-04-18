#ifdef __unix__
#pragma once
#include "IFileSelectorUI.hpp"
#include <string>
#include <vector>

class UnixFileSelectorUI : public IFileSelectorUI {
public:
    UnixFileSelectorUI(const std::string &start, const std::vector<std::string> &exts);
    std::vector<std::string> run() override;

private:
    std::string startPath;
    std::vector<std::string> extensions;
};
#endif // __unix__
