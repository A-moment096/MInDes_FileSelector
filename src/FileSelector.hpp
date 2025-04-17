#pragma once
#include <memory>
#include <string>
#include <vector>

class FileSelector {
public:
    FileSelector(const std::string &start, const std::vector<std::string> &exts);
    ~FileSelector();
    FileSelector() = delete;

    std::vector<std::string> run();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};
