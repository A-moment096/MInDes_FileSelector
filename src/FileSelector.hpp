#pragma once
#include <memory>
#include <string>
#include <vector>
#include <filesystem>
namespace fs = std::filesystem;

class FileSelector {
public:
    FileSelector() = delete;
    FileSelector &operator=(const FileSelector) = delete;
    FileSelector(const FileSelector &) = delete;
    FileSelector &operator=(FileSelector &&) = delete;
    FileSelector(FileSelector &&) = delete;

    FileSelector(const fs::path &start, const std::vector<std::string> &exts);
    ~FileSelector();

    std::vector<fs::path> selectMultipleFile();
    fs::path selectSingleFile();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};
