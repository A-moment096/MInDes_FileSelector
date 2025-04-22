// UIRenderer.hpp
#ifdef __unix__
#pragma once
#include <array>
#include <filesystem>
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/core.h>
#include <set>
#include <string>
#include <vector>
namespace fs = std::filesystem;

class UIRenderer {
public:
    UIRenderer();
    void drawHeader(const fs::path &currentDirectory,
                    const std::vector<std::string> &activeFilters,
                    bool isShowHidden,
                    const std::string &searchName,
                    bool isShowHelp, bool isShowSelected);
    void drawFileList(const std::vector<fs::directory_entry> &entries,
                      size_t cursor,
                      const std::set<fs::path> &selectedMultiPaths);
    void drawFooter(const std::set<fs::path> &selectedMultiPaths, bool showSelected);
    virtual void drawHelp(bool fullHelp);

private:
    std::string getFilterStatus(const std::vector<std::string> &activeFilters);
    std::string getSearchStatus(const std::string &searchName);

    std::string getFormattedFileTime(const fs::directory_entry &entry);
    std::string getFormattedFileSize(const fs::directory_entry &entry);
    std::string getFormattedFileName(const fs::directory_entry &entry, size_t number, bool hasPermission);

    void printFullHelp();
    void printQuickHelp();
};
#endif // __unix__