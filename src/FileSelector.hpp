#pragma once
#include <memory>
#include <string>
#include <vector>

class FileSelector {
public:
    FileSelector() = delete;
    FileSelector &operator=(const FileSelector) = delete;
    FileSelector(const FileSelector &) = delete;
    FileSelector &operator=(FileSelector &&) = delete;
    FileSelector(FileSelector &&) = delete;

    FileSelector(const std::string &start, const std::vector<std::string> &exts);
    ~FileSelector();
    
    std::vector<std::string> run();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};
