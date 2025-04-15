// IFileSelectorUI.hpp
#pragma once

#include <string>
#include <vector>
class IFileSelectorUI {
public:
    virtual ~IFileSelectorUI() = default;
    virtual std::vector<std::string> run() = 0;
};
