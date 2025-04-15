// CommandProcessor.hpp
#pragma once

#include "FileSystemManager.hpp"
#include "UIRenderer.hpp"
#include <set>
#include <string>

class CommandProcessor {
public:
    // Constructor: receives references to the filesystem manager and UI renderer.
    CommandProcessor(FileSystemManager &fsMgr, UIRenderer &renderer);

    // Process a keystroke that is not part of a colon command.
    void processImmediateInput(int key);

    // Process a full command string (that starts with a colon).
    void processCommandInput(const std::string &command);

    int processNumberInput(const std::string &command);

    // Toggle the selection state of the entry at the given index.
    int toggleSelectionAtIndex(size_t index, bool is_multi_selection);

    // Move the cursor up or down.
    void moveCursor(int delta);

    // Get current cursor position (index into the FileSystemManager entries).
    size_t getCursor() const;

    // Checks whether the quit command has been issued.
    bool shouldQuit() const;

    // Access selected file paths.
    const std::set<std::filesystem::path> &getSelectedPaths() const;

    bool isShowHint{false};
    bool isShowHidden{false};

private:
    FileSystemManager &fsManager;
    UIRenderer &uiRenderer;
    size_t cursor;
    bool quit;

    // For simplicity, we store selected files in a set.
    std::set<std::filesystem::path> selectedPaths;

    std::vector<std::string> split_multi_delim(const std::string &input, const std::string &delims);
};
