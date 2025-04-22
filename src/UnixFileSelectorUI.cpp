// UnixFileSelectorUI.cpp
#ifdef __unix__
#include "UnixFileSelectorUI.hpp"
#include "CommandProcessor.hpp"
#include "FileSystemManager.hpp"
#include "KeyEnum.hpp"
#include "TerminalManager.hpp"
#include "UIRenderer.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <sstream>

#include <unistd.h> // for STDIN_FILENO

#include <fmt/color.h>
#include <fmt/core.h>

namespace fs = std::filesystem;
UnixFileSelectorUI::UnixFileSelectorUI(const fs::path &start, const std::vector<std::string> &exts)
    : startPath(start), extensions(exts) {
}

std::vector<fs::path> UnixFileSelectorUI::selectMultipleFile() {
    // Create instances of our components.
    FileSystemManager fsManager(startPath, extensions);
    UIRenderer uiRenderer;
    CommandProcessor cmdProcessor(fsManager, uiRenderer);
    TerminalManager termMgr; // On construction, TerminalManager sets up raw mode.

    // Optional: variables to manage command input mode.
    std::string command_buffer;
    std::string error_message{};

    // Main loop – run until the CommandProcessor signals to quit.
    while (!cmdProcessor.shouldQuit()) {
        fsManager.refreshDirectory(cmdProcessor.isShowHidden);

        uiRenderer.drawHelp(false);
        uiRenderer.drawHeader(fsManager.getCurrentDirectory().string(), fsManager.getFilters(),
                              cmdProcessor.isShowHidden, fsManager.searchName, cmdProcessor.isShowHint, cmdProcessor.isShowSelected);
        uiRenderer.drawFileList(fsManager.getEntries(), cmdProcessor.getCursor(),
                                cmdProcessor.getSelectedPaths());
        if (!error_message.empty()) {
            fmt::print(fg(fmt::color::purple), "\n{}", error_message);
            error_message.clear();
        }
        uiRenderer.drawFooter(cmdProcessor.getSelectedPaths(), cmdProcessor.isShowSelected);
        // Wait for a key press.
        int key = termMgr.readKey();
        try {
            if (key == ':') {
                termMgr.setCanonicalMode();
                fmt::print(fmt::fg(fmt::color::steel_blue),
                           "Command :");
                std::cout.flush();

                command_buffer += termMgr.getLineByChar();
                cmdProcessor.processCommandInput(command_buffer);
                command_buffer.clear();
                termMgr.setRawMode();
            } else if (key >= '0' && key <= '9') {
                termMgr.setCanonicalMode();
                fmt::print(fmt::fg(fmt::color::steel_blue),
                           "Number ");
                std::cout << static_cast<char>(key) << std::flush;

                command_buffer = static_cast<char>(key) + termMgr.getLineByChar();

                cmdProcessor.processNumberInput(command_buffer);
                command_buffer.clear();
                termMgr.setRawMode();
            } else {
                cmdProcessor.processImmediateInput(key);
            }
        } catch (std::invalid_argument &e) {
            command_buffer.clear();
            error_message = e.what();
            termMgr.setRawMode();
        } catch (std::runtime_error &e) {
            command_buffer.clear();
            error_message = e.what();
            termMgr.setRawMode();
        }
    }

    // Restore terminal settings.
    termMgr.restoreTerminal();

    // Prepare the result: convert each selected fs::path into a std::string.
    std::vector<fs::path> selectedFiles;
    for (const auto &path : cmdProcessor.getSelectedPaths()) {
        selectedFiles.push_back(path);
    }
    return selectedFiles;
}
#endif // __unix__