// CommandProcessor.cpp
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
    case '0':
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

void CommandProcessor::processCommandInput(const std::string &command) {
    if (command == "q") {
        quit = true;
    } else if (command.compare(0, 7, "filter ") == 0) {
        // Example: ":filter txt,cpp" splits the list.
        std::string exts = command.substr(7);
        std::vector<std::string> filters;
        std::istringstream iss(exts);
        std::string token;
        // Support splitting by commas (and possible spaces)
        while (std::getline(iss, token, ',')) {
            std::istringstream iss2(token);
            std::string ext;
            while (iss2 >> ext) {
                filters.push_back(ext);
            }
        }
        fsManager.setFilters(filters);
    } else if (command == "help") {
        // Delegate help rendering to the UI Renderer.
        uiRenderer.drawHelp(true);
    } else if (command.compare(0, 5, "sort ") == 0) {
        std::string policy = command.substr(5);
        fsManager.setSortPolicy(policy);
    } else if (command.compare(0, 7, "search ") == 0) {
        std::string searchTerm = command.substr(7);
        // Convert search term to lower-case.
        std::transform(searchTerm.begin(), searchTerm.end(), searchTerm.begin(), ::tolower);
        fsManager.searchName = searchTerm;
    } else {
        // Assume a path jump command.
        fs::path newPath = FileSystemManager::expandTilde(command);
        if (fs::is_directory(newPath)) {
            fsManager.navigateTo(newPath);
            cursor = 0;
        } else {
            const std::string error_message = "Unkown path or command: " + command;
            throw std::invalid_argument(error_message);
        }
    }
}

void CommandProcessor::processNumberInput(const std::string &command) {
    std::istringstream iss(command);
    std::string token;
    bool is_multi_mode = (command.find(',') != std::string::npos) or
                         (command.find(' ') != std::string::npos) or
                         (command.find('-') != std::string::npos);

    int entry_size = (int)fsManager.getEntries().size();

    while (std::getline(iss, token, ',')) {
        size_t dash_position = token.find('-');

        if (dash_position != std::string::npos) {
            // range mode
            int start = std::stoi(token.substr(0, dash_position));
            int end = std::stoi(token.substr(dash_position + 1));
            for (int index = std::max(1, start); index <= std::min(end, entry_size); ++index) {
                toggleSelectionAtIndex(index - 1, is_multi_mode);
            }
        } else {
            // single mode
            int index = std::stoi(token);
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
    if (selectedPaths.find(canonical) != selectedPaths.end()) {
        selectedPaths.erase(canonical);
    } else if (entry.is_regular_file()) {
        selectedPaths.insert(canonical);
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

const std::set<fs::path> &CommandProcessor::getSelectedPaths() const {
    return selectedPaths;
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