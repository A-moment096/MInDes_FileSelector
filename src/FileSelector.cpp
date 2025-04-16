// FileSelector.cpp
#ifdef __unix__
#include "FileSelector.hpp"
#ifdef __unix__
#include "UnixFileSelectorUI.hpp"
#elif defined(_WIN32)
#include "WindowsFileSelectorUI.hpp"
#endif

FileSelector::FileSelector(const std::string &start, const std::vector<std::string> &exts) {
#ifdef __unix__
    uiImpl = std::make_unique<UnixFileSelectorUI>(start, exts);
#elif defined(_WIN32)
    uiImpl = std::make_unique<WindowsFileSelectorUI>(start, exts);
#endif
}

std::vector<std::string> FileSelector::run() {
    return uiImpl->run();
}
#endif // __unix__
