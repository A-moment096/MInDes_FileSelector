// UIRenderer.hpp
#pragma once
#include <array>
#include <filesystem>
#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/chrono.h>
#include <set>
#include <string>
#include <vector>
namespace fs = std::filesystem;

class UIRenderer {
public:
    UIRenderer();
    void drawHeader(const std::string &currentDirectory,
                    const std::vector<std::string> &activeFilters,
                    bool showHidden,
                    const std::string &searchName,
                    bool showHelp);
    void drawFileList(const std::vector<fs::directory_entry> &entries,
                      size_t cursor,
                      const std::set<fs::path> &selectedPaths);
    void drawFooter(const std::set<fs::path> &selectedPaths, bool showSelected);
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
