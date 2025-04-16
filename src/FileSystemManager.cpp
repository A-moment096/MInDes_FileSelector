// FileSystemManager.cpp
#ifdef __unix__
#include "FileSystemManager.hpp"
#include <cstdlib>
#include <fmt/core.h>
#include <iostream>
#include <sstream>

FileSystemManager::FileSystemManager(const fs::path &startDirectory,
                                     const std::vector<std::string> &filters)
    : currentDirectory(fs::canonical(expandTilde(startDirectory))),
      previousDirectory(currentDirectory), filters(filters) {}

void FileSystemManager::refreshDirectory(bool is_show_hidden) {
    bool is_dir_changed = (previousDirectory != currentDirectory);
    if (is_dir_changed) {
        previousDirectory = currentDirectory;
    }
    entries.clear();
    try {
        for (const auto &entry : fs::directory_iterator(currentDirectory)) {
            std::string filename = entry.path().filename().string();
            bool is_hidden = (!filename.empty() && filename[0] == '.');
            bool include = false;
            if (entry.is_directory()) {
                include = is_show_hidden || !is_hidden;
            } else if (entry.is_regular_file()) {
                include = matchesFilter(entry.path()) && (is_show_hidden || !is_hidden);
            }
            if (include) {
                entries.push_back(entry);
            }
        }
        sortEntries();
        search();
    } catch (...) {
        // Optionally, log errors here.
    }
}

void FileSystemManager::setSortPolicy(const std::string &policy) {
    sortPolicy.clear();
    commandStringParser(sortPolicy, policy);
}

void FileSystemManager::setFilters(const std::string &exts) {
    filters.clear();
    commandStringParser(filters, exts);
}

void FileSystemManager::search() {
    if (searchName.empty()) {
        return;
    }
    std::vector<Entry> search_results;
    for (const auto &entry : entries) {
        std::string entry_filename = entry.path().filename().string();
        std::string lowered = entry_filename;
        std::transform(lowered.begin(), lowered.end(), lowered.begin(), ::tolower);
        if (lowered.find(searchName) != std::string::npos) {
            search_results.push_back(entry);
        }
    }
    entries = search_results;
}

void FileSystemManager::navigateParent() {
    currentDirectory = currentDirectory.parent_path();
}

void FileSystemManager::navigateTo(const fs::path &newPath) {
    if (fs::is_directory(newPath)) {
        currentDirectory = fs::canonical(newPath);
    }
}

bool FileSystemManager::matchesFilter(const fs::path &p) const {
    if (filters.empty())
        return true;
    std::string ext = p.extension().string();
    if (!ext.empty()) {
        ext = ext.substr(1);
    }
    return std::find(filters.begin(), filters.end(), ext) != filters.end();
}

void FileSystemManager::commandStringParser(std::vector<std::string> &vector, const std::string &str) {
    std::istringstream iss(str);
    std::string token;
    // Support splitting by commas (and possible spaces)
    while (std::getline(iss, token, ',')) {
        std::istringstream iss2(token);
        std::string ext;
        while (iss2 >> ext) {
            vector.push_back(ext);
        }
    }
}

void FileSystemManager::sortEntries() {
    // Map of comparators.
    Comparator cmpDirFirst = [](const Entry &a, const Entry &b) {
        return a.is_directory() && !b.is_directory();
    };
    Comparator cmpName = [](const Entry &a, const Entry &b) {
        return a.path().filename() < b.path().filename();
    };
    Comparator cmpTime = [](const Entry &a, const Entry &b) {
        return fs::last_write_time(a.path()) > fs::last_write_time(b.path());
    };
    Comparator cmpExtension = [](const Entry &a, const Entry &b) {
        return a.path().extension() < b.path().extension();
    };
    Comparator cmpSize = [](const Entry &a, const Entry &b) {
        auto sizeA = fs::is_regular_file(a) ? fs::file_size(a) : 0;
        auto sizeB = fs::is_regular_file(b) ? fs::file_size(b) : 0;
        return sizeA < sizeB;
    };

    std::unordered_map<std::string, Comparator> comparatorMap = {
        {"dir", cmpDirFirst},
        {"name", cmpName},
        {"time", cmpTime},
        {"type", cmpExtension},
        {"size", cmpSize}};

    std::vector<Comparator> sorters;
    for (auto const &token : sortPolicy) {
        if (auto it = comparatorMap.find(token); it != comparatorMap.end()) {
            sorters.push_back(it->second);
        } else {
            std::cerr << "Unknown sort key: " << token << std::endl;
        }
    }
    auto finalComparator = combineComparators(sorters);
    std::sort(entries.begin(), entries.end(), finalComparator);
}

FileSystemManager::Comparator FileSystemManager::combineComparators(const std::vector<Comparator> &comps) {
    return [=](const Entry &a, const Entry &b) {
        for (const auto &comp : comps) {
            if (comp(a, b))
                return true;
            if (comp(b, a))
                return false;
        }
        return false;
    };
}

fs::path FileSystemManager::expandTilde(const fs::path &path) {
    std::string str = path.string();
    auto replaceAll = [](std::string str, const std::string &from, const std::string &to) -> std::string {
        size_t pos = str.find(from, 0);
        while (pos != std::string::npos) {
            str.replace(pos, from.length(), to);
            pos = str.find(from, pos + to.length());
        }
        return str;
    };
    if (str.find("~") != std::string::npos) {
        const char *home = std::getenv("HOME");
        if (home)
            str = replaceAll(str, "~", home);
        else
            str = replaceAll(str, "~", "/");
        return fs::canonical(str);
    }
    return path;
}
#endif // __unix__