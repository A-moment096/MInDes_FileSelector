// CommandProcessor.cpp
#ifdef __unix__
#include "CommandProcessor.hpp"
#include "KeyEnum.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <sstream>

// Assume a namespace alias for filesystem:
namespace fs = std::filesystem;

CommandProcessor::CommandProcessor(FileSystemManager &fsMgr, UIRenderer &renderer)
    : fsManager(fsMgr), uiRenderer(renderer), cursor(0), quit(false) {}

void CommandProcessor::processImmediateInput(int key) {
    // Immediate mode: navigation and selection commands.
    switch (key) {
    case KEY_ARROW_LEFT:
    case 'h':
    case 127: // Backspace
        // Navigate to parent directory.
        fsManager.navigateParent();
        cursor = 0;
        break;
    case KEY_ARROW_RIGHT:
    case '\n':
    case '\r':
    case 'l':
    case ' ': {
        const auto &entries = fsManager.getEntries();
        if (cursor < entries.size()) {
            const auto &entry = entries[cursor];
            if (entry.is_directory()) {
                fsManager.navigateTo(entry.path());
                cursor = 0;
            } else if (entry.is_regular_file()) {
                // Toggle selection if a regular file.
                toggleSelectionAtIndex(cursor, false);
            }
        }
    } break;
    case KEY_ARROW_UP:
    case 'k':
        moveCursor(-1);
        break;
    case KEY_ARROW_DOWN:
    case 'j':
        moveCursor(1);
        break;
    case 'q':
        quit = true;

        break;
    case '!':
        isShowHint = !isShowHint;
        break;
    case '?':
        uiRenderer.drawHelp(true);
        break;
    case 'H':
        isShowHidden = !isShowHidden;
        break;
    case 'S':
        isShowSelected = !isShowSelected;
        break;
    default:
        throw std::invalid_argument("Invalid Input");
        break;
    }
}

void CommandProcessor::processImmediateInputSingle(int key) {
    // Immediate mode: navigation and selection commands.
    switch (key) {
    case KEY_ARROW_LEFT:
    case 'h':
    case 127: // Backspace
        // Navigate to parent directory.
        fsManager.navigateParent();
        cursor = 0;
        break;
    case KEY_ARROW_RIGHT:
    case '\n':
    case '\r':
    case 'l':
    case ' ': {
        const auto &entries = fsManager.getEntries();
        if (cursor < entries.size()) {
            const auto &entry = entries[cursor];
            if (entry.is_directory()) {
                fsManager.navigateTo(entry.path());
                cursor = 0;
            } else if (entry.is_regular_file()) {
                // Toggle selection if a regular file.
                toggleSelectionAtIndexSingle(cursor);
            }
        }
    } break;
    case KEY_ARROW_UP:
    case 'k':
        moveCursor(-1);
        break;
    case KEY_ARROW_DOWN:
    case 'j':
        moveCursor(1);
        break;
    case 'q':
        quit = true;

        break;
    case '!':
        isShowHint = !isShowHint;
        break;
    case '?':
        uiRenderer.drawHelp(true);
        break;
    case 'H':
        isShowHidden = !isShowHidden;
        break;
    case 'S':
        isShowSelected = !isShowSelected;
        break;
    default:
        throw std::invalid_argument("Invalid Input");
        break;
    }
}

void CommandProcessor::processCommandInput(const std::string &command) {
    std::stringstream command_stream{command};
    std::string command_token{};
    const auto trim_whitespace = [](const std::string &s) {
        size_t start = s.find_first_not_of(" \t\n\r");
        if (start == std::string::npos)
            return std::string();

        size_t end = s.find_last_not_of(" \t\n\r");
        return s.substr(start, end - start + 1);
    };
    // std::getline(command_stream, command_token, ' ');
    command_stream >> command_token;
    if (command_token == "q" or command_token == "Q") {
        quit = true;
    } else if (command_token == "filter") {
        std::string exts{};
        std::getline(command_stream, exts);
        fsManager.setFilters(exts);
    } else if (command_token == "sort") {
        std::string policy;
        std::getline(command_stream, policy);
        fsManager.setSortPolicy(policy);
    } else if (command_token == "search") {
        std::string search_name;
        std::getline(command_stream, search_name);
        if (!search_name.empty()) {
            std::transform(search_name.begin(), search_name.end(), search_name.begin(), ::tolower);
            fsManager.searchName = trim_whitespace(search_name); // Trim the leading space
        } else {
            fsManager.searchName = search_name;
        }
    } else if (command_token == "help") {
        uiRenderer.drawHelp(true);
    } else {
        // Assume a path jump command.
        fs::path newPath = FileSystemManager::expandTilde(command);
        if (fs::is_directory(newPath)) {
            fsManager.navigateTo(newPath);
            cursor = 0;
        } else {
            if (!command.empty()) {
                const std::string error_message = "Unkown path or command: " + command;
                throw std::invalid_argument(error_message);
            }
        }
    }
}

void CommandProcessor::processNumberInput(const std::string &command) {
    std::istringstream iss(command);
    std::string token;
    bool is_multi_mode = (command.find(',') != std::string::npos) or
                         (command.find(' ') != std::string::npos) or
                         (command.find('-') != std::string::npos);

    size_t entry_size = fsManager.getEntries().size();

    while (std::getline(iss, token, ',')) {
        size_t dash_position = token.find('-');

        if (dash_position != std::string::npos) {
            // range mode
            size_t start = std::stoi(token.substr(0, dash_position));
            size_t end = std::stoi(token.substr(dash_position + 1));
            for (int index = std::max(static_cast<size_t>(1), start); index <= std::min(end, entry_size); ++index) {
                toggleSelectionAtIndex(index - 1, is_multi_mode);
            }
        } else {
            // single mode
            size_t index = std::stoi(token);
            if (index >= 1 && index <= entry_size) {
                toggleSelectionAtIndex(index - 1, is_multi_mode);
            }
        }
    }
}

void CommandProcessor::processNumberInputSingle(const std::string &command) {
    std::istringstream iss(command);
    std::string token;
    bool is_multi_mode{false}; // In single file selection mode

    size_t entry_size = fsManager.getEntries().size();

    while (std::getline(iss, token, ',')) {
        size_t dash_position = token.find('-');

        if (dash_position != std::string::npos) {
            throw std::invalid_argument("Single file selection mode, please input one single number only");
        } else {
            // single mode
            size_t index = std::stoi(token);
            if (index >= 1 && index <= entry_size) {
                toggleSelectionAtIndex(index - 1, is_multi_mode);
            }
        }
    }
}

void CommandProcessor::toggleSelectionAtIndex(size_t index, bool is_multi_selection) {
    const auto &entries = fsManager.getEntries();
    if (index >= entries.size()) {
        return;
    }
    const auto &entry = entries[index];
    fs::path canonical = fs::canonical(entry.path());
    // Toggle selection: if already selected, unselect it.
    if (selectedMultiPaths.find(canonical) != selectedMultiPaths.end()) {
        selectedMultiPaths.erase(canonical);
    } else if (entry.is_regular_file()) {
        selectedMultiPaths.insert(canonical);
    } else if (entry.is_directory()) {
        if (!is_multi_selection) {
            fsManager.navigateTo(entry.path());
            cursor = 0;
        } else {
            throw std::invalid_argument("Can't open a directory in range mode ");
        }
    } else {
        throw std::runtime_error("Invalid entry detected");
    }
}

void CommandProcessor::toggleSelectionAtIndexSingle(size_t index) {
    const auto &entries = fsManager.getEntries();
    if (index >= entries.size()) {
        return;
    }
    const auto &entry = entries[index];
    fs::path canonical = fs::canonical(entry.path());
    // Toggle selection: only one file can be selected
    
    if (selectedSinglePath == canonical) {
        selectedSinglePath.clear();
    } else if (entry.is_regular_file()) {
        selectedSinglePath = canonical;

    } else if (entry.is_directory()) {
        fsManager.navigateTo(entry.path());
        cursor = 0;
    } else {
        throw std::runtime_error("Invalid entry detected");
    }
}

void CommandProcessor::moveCursor(int delta) {
    size_t count = fsManager.getEntries().size();
    if (count == 0)
        return;
    int newCursor = static_cast<int>(cursor) + delta;
    // Simple wrap-around logic:
    if (newCursor < 0)
        newCursor = count - 1;
    else if (newCursor >= static_cast<int>(count))
        newCursor = 0;
    cursor = static_cast<size_t>(newCursor);
}

size_t CommandProcessor::getCursor() const {
    return cursor;
}

bool CommandProcessor::shouldQuit() const {
    return quit;
}

const std::set<fs::path> &CommandProcessor::getSelectedMultiPaths() const {
    return selectedMultiPaths;
}

const fs::path &CommandProcessor::getSelectedSinglePath() const {
    return selectedSinglePath;
}

std::vector<std::string> CommandProcessor::split_multi_delim(const std::string &input, const std::string &delims) {
    std::vector<std::string> result;
    std::string token;

    for (char ch : input) {
        if (delims.find(ch) != std::string::npos) {
            if (!token.empty()) {
                result.push_back(token);
                token.clear();
            }
        } else {
            token += ch;
        }
    }

    if (!token.empty()) {
        result.push_back(token);
    }

    return result;
}
#endif // __unix__