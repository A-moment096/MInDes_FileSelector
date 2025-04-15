// FileSystemManager.hpp
#pragma once
#include <algorithm>
#include <filesystem>
#include <functional>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

class FileSystemManager {
public:
    using Entry = fs::directory_entry;
    using Comparator = std::function<bool(const Entry &, const Entry &)>;

    FileSystemManager(const fs::path &startDirectory,
                      const std::vector<std::string> &filters = {});

    void refreshDirectory(bool showHidden);
    void setSortPolicy(const std::string &policy);
    void setFilters(const std::vector<std::string> &exts);
    void search();

    const std::vector<Entry> &getEntries() const { return entries; }
    fs::path getCurrentDirectory() const { return currentDirectory; }
    const std::vector<std::string> &getFilters() const { return filters; }
    void navigateParent();
    void navigateTo(const fs::path &newPath);

    // Utility: expands tilde in paths.
    static fs::path expandTilde(const fs::path &path);
    std::string searchName;

private:
    fs::path currentDirectory;
    fs::path previousDirectory;
    std::vector<std::string> filters;
    std::vector<Entry> entries;
    std::string sortPolicy = "dir,type,name";

    void sortEntries();
    Comparator combineComparators(const std::vector<Comparator> &comps);
    bool matchesFilter(const fs::path &p) const;
};
