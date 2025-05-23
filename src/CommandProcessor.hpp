// CommandProcessor.hpp
#ifdef __unix__
#pragma once
#include "FileSystemManager.hpp"
#include "UIRenderer.hpp"
#include <set>
#include <string>
#include <vector>
namespace fs = std::filesystem;

class CommandProcessor {
public:
    // Constructor: receives references to the filesystem manager and UI renderer.
    CommandProcessor(FileSystemManager &fsMgr, UIRenderer &renderer);

    // Process a keystroke that is not part of a colon command.
    void processImmediateInput(int key);

    void processImmediateInputSingle(int key);

    // Process a full command string (that starts with a colon).
    void processCommandInput(const std::string &command);

    void processNumberInput(const std::string &command);

    void processNumberInputSingle(const std::string &command);

    // Toggle the selection state of the entry at the given index.
    void toggleSelectionAtIndex(size_t index, bool is_multi_selection);

    void toggleSelectionAtIndexSingle(size_t index);

    // Move the cursor up or down.
    void moveCursor(int delta);

    // Get current cursor position (index into the FileSystemManager entries).
    size_t getCursor() const;

    // Checks whether the quit command has been issued.
    bool shouldQuit() const;

    // Access selected file path(s).
    const std::set<fs::path> &getSelectedMultiPaths() const;
    const fs::path &getSelectedSinglePath() const;

    bool isShowHint{false};
    bool isShowHidden{false};
    bool isShowSelected{true};

private:
    FileSystemManager &fsManager;
    UIRenderer &uiRenderer;
    size_t cursor;
    bool quit;

    std::set<fs::path> selectedMultiPaths;
    fs::path selectedSinglePath;

    std::vector<std::string> split_multi_delim(const std::string &input, const std::string &delims);
};
#endif // __unix__