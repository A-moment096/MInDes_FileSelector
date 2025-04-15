// UnixFileSelectorUI.cpp
#include "UnixFileSelectorUI.hpp"
#include "CommandProcessor.hpp"
#include "FileSystemManager.hpp"
#include "KeyEnum.hpp"
#include "TerminalManager.hpp"
#include "UIRenderer.hpp"

#include <chrono>
#include <fmt/color.h>
#include <fmt/core.h>
#include <iostream>
#include <sstream>
#include <unistd.h> // for STDIN_FILENO

UnixFileSelectorUI::UnixFileSelectorUI(const std::string &start, const std::vector<std::string> &exts)
    : startPath(start), extensions(exts) {
    // Nothing else is needed here;
    // The individual components will be instantiated in run().
}

std::vector<std::string> UnixFileSelectorUI::run() {
    // Create instances of our components.
    FileSystemManager fsManager(startPath, extensions);
    UIRenderer uiRenderer;
    CommandProcessor cmdProcessor(fsManager, uiRenderer);
    TerminalManager termMgr; // On construction, TerminalManager sets up raw mode.

    // Optional: variables to manage command input mode.
    bool commandMode = false;
    std::string commandBuffer;
    int ERROR_CODE = 0;

    // Main loop – run until the CommandProcessor signals to quit.
    while (!cmdProcessor.shouldQuit()) {
        // Refresh entries from the current directory (pass 'true' or 'false' for showHidden).
        // You might add a toggle for hidden files; for now we assume false.
        fsManager.refreshDirectory(cmdProcessor.isShowHidden);

        uiRenderer.drawHelp(false);
        uiRenderer.drawHeader(fsManager.getCurrentDirectory().string(), fsManager.getFilters(),
                              cmdProcessor.isShowHidden, fsManager.searchName, cmdProcessor.isShowHint);
        uiRenderer.drawFileList(fsManager.getEntries(), cmdProcessor.getCursor(),
                                cmdProcessor.getSelectedPaths());
        printErr(ERROR_CODE);
        uiRenderer.drawFooter(cmdProcessor.getSelectedPaths(), /* showSelected */ true);
        // Wait for a key press.
        int key = termMgr.readKey();

        // If user enters colon, switch to command mode.
        if (key == ':') {
            // Switch to canonical mode to read a full command.
            termMgr.setCanonicalMode();
            fmt::print(fmt::fg(fmt::color::steel_blue),
                       "Command :");
            std::cout.flush();

            commandBuffer += termMgr.getLineByChar();
            // Process the command.
            cmdProcessor.processCommandInput(commandBuffer);
            commandBuffer.clear();
            // Revert back to raw mode.
            termMgr.setRawMode();
        } else if (key >= '1' && key <= '9') {
            termMgr.setCanonicalMode();
            fmt::print(fmt::fg(fmt::color::steel_blue),
                       "Number ");
            std::cout << static_cast<char>(key) << std::flush;

            commandBuffer = static_cast<char>(key) + termMgr.getLineByChar();

            ERROR_CODE = cmdProcessor.processNumberInput(commandBuffer);
            commandBuffer.clear();
            termMgr.setRawMode();
        }
        // In our design, if a positive digit is pressed, it could be a selection command.
        // For simplicity, we treat all non-special keys as immediate navigation commands.
        else {
            cmdProcessor.processImmediateInput(key);
        }
    }

    // Restore terminal settings.
    termMgr.restoreTerminal();

    // Prepare the result: convert each selected fs::path into a std::string.
    std::vector<std::string> selectedFiles;
    for (const auto &path : cmdProcessor.getSelectedPaths()) {
        selectedFiles.push_back(path.string());
    }
    return selectedFiles;
}

void UnixFileSelectorUI::printErr(int ERROR_CODE) {
    if (ERROR_CODE == 1) {
        std::cout << "Can't open a directory in range mode\n";
        ERROR_CODE = 0;
    }
}