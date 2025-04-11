#pragma once

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#ifdef __unix__
#include <termios.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/ostream.h>
namespace fs = std::filesystem;

class FileSelector {
public:
#ifdef __unix__
    FileSelector(const std::string &start, const std::vector<std::string> &exts)
        : currentPath(expandTilde(fs::canonical(start))),
          filters(exts), cursor(0),
          commandMode(false), showHelp(true) {}
#elif defined(_WIN32)
    FileSelector(const std::string &start = ".", const std::vector<std::string> &exts = {})
        : currentPath(fs::canonical(start)),
          filters(exts) {}

#endif //__unix__

    std::vector<std::string> run() {
#ifdef __unix__
        return linuxRun();
#endif //__unix__

#ifdef _WIN32
        // Windows implementation can be added here
        return windowsRun();
#endif // _WIN32
    }

private:
    fs::path currentPath;
    std::vector<std::string> filters;
#ifdef __unix__
    // State management
    std::vector<fs::directory_entry> entries;
    size_t cursor;
    std::set<fs::path> selectedPaths;
    termios originalTermios{};
    bool commandMode;
    bool showHelp;
    bool inRangeMode;
    bool openFolderinRange;
    bool showFullHelp = false;
    bool showHidden = false;
    bool shouldQuit = false;
    std::string commandBuffer;

    std::vector<std::string> linuxRun() {
        setupTerminal();
        while (!shouldQuit) {
            refreshEntries();
            drawInterface();
            processInput();
        }
        restoreTerminal();
        return {selectedPaths.begin(), selectedPaths.end()};
    }

    // Terminal management
    void setupTerminal() {
        tcgetattr(STDIN_FILENO, &originalTermios);
        termios raw = originalTermios;
        raw.c_lflag &= ~(ECHO | ICANON);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    }

    void restoreTerminal() {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalTermios);
    }

    // Path processing
    static fs::path expandTilde(const fs::path &path) {
        std::string str = path.string();
        auto replaceAll = [](std::string str, const std::string &from, const std::string &to) -> std::string {
            auto &&pos = str.find(from, size_t{});
            while (pos != std::string::npos) {
                str.replace(pos, from.length(), to);
                pos = str.find(from, pos + to.length());
            }
            return str;
        };
        if (str.find("~") != std::string::npos) {
            const char *home = std::getenv("HOME");
            if (home) {
                str = replaceAll(str, "~", home);
            } else {
                str = replaceAll(str, "~", "/");
            }
            return fs::canonical(str);
        }
        return path;
    }

    // File filtering
    bool matchesFilter(const fs::path &p) {
        if (filters.empty())
            return true;
        std::string ext = p.extension().string();
        return std::find(filters.begin(), filters.end(), ext) != filters.end();
    }

    // Entry processing
    void refreshEntries() {
        entries.clear();
        try {
            for (const auto &entry : fs::directory_iterator(currentPath)) {
                std::string filename = entry.path().filename().string();
                bool isHidden = !filename.empty() && filename[0] == '.';

                bool include = false;
                if (entry.is_directory()) {
                    include = showHidden || !isHidden;
                } else if (entry.is_regular_file()) {
                    include = matchesFilter(entry.path()) &&
                              (showHidden || !isHidden);
                }
                if (include) {
                    entries.push_back(entry);
                }
            }
        } catch (...) {
        }

        std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) {
            return a.path().filename() < b.path().filename();
        });

        cursor = std::min(cursor, entries.empty() ? 0 : entries.size() - 1);
    }

    // Input handling
    void processInput() {
        char key = readKey();
        if (commandMode) {
            handleCommandInput(key);
        } else {
            handleNormalInput(key);
        }
    }

    char readKey() {
        char c;
        read(STDIN_FILENO, &c, 1);
        return c;
    }

    void handleCommandInput(char key) {
        if (key == '\n') {
            executeCommand();
            commandMode = false;
            commandBuffer.clear();
        } else if (key == 127 || key == '\b') {
            if (!commandBuffer.empty())
                commandBuffer.pop_back();
        } else if (key >= 32 && key <= 126) {
            commandBuffer += key;
        }
    }

    void handleNormalInput(char key) {
        switch (key) {
        case 'q':
            shouldQuit = true;
            break;
        case 'j':
            moveCursor(1);
            break;
        case 'k':
            moveCursor(-1);
            break;
        case 'h':
        case '0':
            navigateParent();
            break;
        case 'l':
        case ' ':
            toggleSelection_navigateChild();
            break;
        case ':':
            startCommand(":");
            break;
        case '\n':
            confirmSelection();
            break;
        case '!':
            showHelp = !showHelp;
            break;
        case '?':
            showFullHelp = true;
            break;
        case 'H':
            showHidden = !showHidden;
            break;
        default:
            if (isdigit(key))
                startCommand(std::string(1, key));
            break;
        }
    }

    // Navigation commands
    void moveCursor(int delta) {
        if (entries.empty())
            return;
        cursor = (cursor + entries.size() + delta) % entries.size();
    }

    void navigateParent() {
        currentPath = currentPath.parent_path();
        cursor = 0;
    }

    // Selection commands
    void toggleSelection_navigateChild() {
        if (entries.empty())
            return;
        const auto &entry = entries[cursor];
        fs::path canonical = fs::canonical(entry.path());
        if (selectedPaths.count(canonical)) {
            selectedPaths.erase(canonical);
        } else if (entry.is_regular_file()) {
            selectedPaths.insert(canonical);
        } else if (entry.is_directory()) {
            currentPath = fs::canonical(entry.path());
            cursor = 0;
        }
    }

    void confirmSelection() {
        shouldQuit = true;
    }

    // Command processing
    void startCommand(const std::string &prefix) {
        commandMode = true;
        commandBuffer = prefix;
    }

    void executeCommand() {
        if (commandBuffer.empty())
            return;

        if (commandBuffer[0] == ':') {
            handleColonCommand();
        } else {
            handleSelectionCommand();
        }
    }

    void handleColonCommand() {
        // commandBuffer.erase(
        //     remove_if(
        //         commandBuffer.begin(), commandBuffer.end(), isspace),
        //     commandBuffer.end());

        if (commandBuffer == ":q") {
            shouldQuit = true;
        } else if (commandBuffer.find(":filter ") != std::string::npos) {
            handleColonAddFilter();
        } else if (commandBuffer == ":help") {
            showFullHelp = true;
        } else {
            handleColonPathCommand();
        }
    }

    void handleColonAddFilter() {
        std::string exts = commandBuffer.substr(8);
        std::istringstream iss(exts);
        std::string ext;
        while (std::getline(iss, ext, ',')) {
            filters.push_back("." + ext); // Auto-add dot prefix
        }
    }

    void handleColonPathCommand() {
        try {
            fs::path newPath = expandTilde(commandBuffer.substr(1));
            if (fs::is_directory(newPath)) {
                currentPath = fs::canonical(newPath);
                cursor = 0;
            }
        } catch (...) {
        }
    }

    void handleSelectionCommand() {
        std::istringstream iss(commandBuffer);
        std::string token;

        if (commandBuffer.find(',') != std::string::npos) {
            inRangeMode = true;
        }
        while (getline(iss, token, ',')) {
            processSelectionToken(token);
        }
    }

    void processSelectionToken(const std::string &token) {
        size_t dashPos = token.find('-');
        if (dashPos != std::string::npos) {
            inRangeMode = true;
            processRangeSelection(token, dashPos);
        } else {
            processSingleSelection(token);
        }
    }

    void processRangeSelection(const std::string &token, size_t dashPos) {
        try {
            int start = std::stoi(token.substr(0, dashPos));
            int end = std::stoi(token.substr(dashPos + 1));
            for (int i = std::max(1, start); i <= std::min(end, (int)entries.size()); ++i) {
                toggleIndexSelection(i - 1);
            }
        } catch (...) {
        }
    }

    void processSingleSelection(const std::string &token) {
        try {
            int index = std::stoi(token);
            if (index >= 1 && index <= (int)entries.size()) {
                toggleIndexSelection(index - 1);
            }
        } catch (...) {
        }
    }

    void toggleIndexSelection(size_t index) {
        if (index >= entries.size())
            return;
        const auto &entry = entries[index];
        fs::path canonical = fs::canonical(entry.path());
        if (selectedPaths.count(canonical)) {
            selectedPaths.erase(canonical);
        } else if (entry.is_regular_file()) {
            selectedPaths.insert(canonical);
        } else if (entry.is_directory()) {
            if (!inRangeMode) {
                currentPath = fs::canonical(entry.path());
                cursor = 0;
            } else {
                openFolderinRange = true;
            }
        }
    }

    // Interface rendering
    void drawInterface() {
        fmt::print("\033[2J\033[H"); // Clear screen
        if (showFullHelp) {
            printFullHelp();
            showFullHelp = false;
            fmt::print(fg(fmt::color::steel_blue) | fmt::emphasis::bold,
                       "\nPress any key to continue...\n");
            (void)getchar();
            fmt::print("\033[2J\033[H");
        }
        drawHeader();
        drawFileList();
        drawFooter();
    }

    void drawHeader() {
        const auto header_style = fmt::emphasis::bold | fg(fmt::color::light_blue);
        const auto hidden_style = showHidden ? fg(fmt::color::green) : fg(fmt::color::red);
        const auto title_style = fmt::emphasis::bold | fg(fmt::color::gold);

        if (showHelp) {
            fmt::print(title_style, "\n{:-^60}\n", " HELP ");
            fmt::print(fg(fmt::color::light_gray),
                       "\n"
                       "Navigation:\n"
                       "  {:<6} - Move up       {:<6} - Move down\n"
                       "  {:<6} - Parent dir   {:<6} - Enter dir\n"
                       "Selection:\n"
                       "  {:<6} - Toggle       {:<6} - Multi-select\n"
                       "Tools:\n"
                       "  {:<6} - Path jump    {:<6} - Toggle this help\n"
                       "  {:<6} - Full help    {:<6} - Quit\n",
                       "↑/k", "↓/j",
                       "←/h", "→/l",
                       "Space", "Numbers",
                       ":", "!", "?", "q");
            fmt::print(title_style, "\n{:-^60}\n", "");
        }
        fmt::print("\n");

        fmt::print(header_style, "📁 {}\n", currentPath.string());
        fmt::print(hidden_style, "[Hidden Folder: {}] ", showHidden ? "SHOWN" : "HIDDEN");
        fmt::print("\n");
    }

    void drawFileList() {
        const auto dir_style = fg(fmt::color::deep_sky_blue);
        const auto file_style = fg(fmt::color::white);

        for (size_t i = 0; i < entries.size(); ++i) {
            const auto &entry = entries[i];
            const bool selected = selectedPaths.count(fs::canonical(entry.path()));

            fmt::print(i == cursor ? "▶ " : "  ");
            fmt::print(selected ? "[✓] " : "[ ] ");

            if (entry.is_directory()) {
                fmt::print(dir_style | fmt::emphasis::bold, "📁 ");
                fmt::print(dir_style, "{:3} {}\n",
                           i + 1, entry.path().filename().string());
            } else {
                fmt::print(file_style, "📄 ");
                fmt::print(file_style, "{:3} {}\n",
                           i + 1, entry.path().filename().string());
            }
        }
    }

    void drawFooter() {
        if (commandMode) {
            std::cout << "\nCommand: " << commandBuffer << "_";
        }
        if (openFolderinRange) {
            std::cout << "Can't open a directory in range mode\n";
            openFolderinRange = false;
        }
        std::cout << "\nSelected: " << selectedPaths.size() << " files\n";
        for (auto &f : selectedPaths)
            std::cout << " - " << f.filename() << "\n";
    }

    void printFullHelp() {
        const auto title_style = fmt::emphasis::bold | fg(fmt::color::gold);
        const auto section_style = fg(fmt::color::aqua) | fmt::emphasis::underline;
        const auto warn_style = fg(fmt::color::orange) | fmt::emphasis::bold;

        fmt::print(title_style, "\n{:-^60}\n", " HELP ");
        fmt::print(section_style, "\nNavigation:\n");
        fmt::print("  {:20} {}\n", "↑/k", "Move up");
        fmt::print("  {:20} {}\n", "↓/j", "Move down");
        fmt::print("  {:20} {}\n", "←/h/0", "Parent directory");
        fmt::print("  {:20} {}\n", "→/l/Space", "[📁]: Enter directory");
        fmt::print("  {:20} {}\n", "", "[📄]: Toggle file");

        fmt::print(section_style, "\nNumber Operation:\n");
        fmt::print("  {:20} {}\n", "3+Enter", "[📁]: Enter directory");
        fmt::print("  {:20} {}\n", "", "[📄]: Select by number");
        fmt::print(warn_style, "{}\n", "Following two method won't enter directories");
        fmt::print("  {:20} {}\n", "1-9+Enter", "Select a range of files");
        fmt::print("  {:20} {}\n", "1-5,8+Enter", "Multiple selection");

        fmt::print(section_style, "\nAdvanced:\n");
        fmt::print("  {:20} {}\n", ":path", "Jump to path");
        fmt::print("  {:20} {}\n", ":filter txt,cpp", "Set multiple filters");
        fmt::print("  {:20} {}\n", "H", "Toggle hidden files");

        fmt::print(section_style, "\nAuxiliary:\n");
        fmt::print("  {:20} {}\n", "q/Enter", "Finish selection");
        fmt::print("  {:20} {}\n", "!", "Toggle quick help");
        fmt::print("  {:20} {}\n", "?", "Print this full help");

        fmt::print(title_style, "\n{:-^60}\n", "");
    }
#endif

#ifdef _WIN32
    std::string windowTitle{};

    std::vector<std::string> windowsRun() {
        // Allocate large buffer to hold multiple file names
        const DWORD bufferSize = 65536; // 64 KB
        char *szBuffer = new char[bufferSize];
        ZeroMemory(szBuffer, bufferSize);

        OPENFILENAMEA ofn = {0};
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrInitialDir = currentPath.string().c_str();

        std::string filter{};
        for (const auto &ext : filters) {
            std::string ext_name = ext;
            ext_name.erase(0, ext_name.find_first_of('.') + 1);
            std::transform(ext_name.begin(), ext_name.end(), ext_name.begin(), ::toupper);
            filter += ext_name + " Files (*" + ext + ")";
            filter.push_back('\0');
            filter += "*" + ext;
            filter.push_back('\0');
        }
        filter += "All Files (*.*)";
        filter.push_back('\0');
        filter += "*.*";
        filter.push_back('\0');
        ofn.lpstrFilter = filter.c_str();

        // ofn.lpstrTitle = "Select MInDes Input File(s)";
        ofn.lpstrFile = szBuffer;
        ofn.nMaxFile = bufferSize;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER |
                    OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT;

        std::vector<std::string> selectedFiles;

        if (GetOpenFileNameA(&ofn)) {
            char *p = szBuffer;
            std::string directory = p;
            p += directory.size() + 1;

            if (*p == '\0') {
                // Only one file selected
                selectedFiles.push_back(directory);
            } else {
                // Multiple files selected
                while (*p) {
                    std::string filename = p;
                    selectedFiles.push_back(directory + "\\" + filename);
                    p += filename.size() + 1;
                }
            }
        } else {
            DWORD dwError = CommDlgExtendedError();
            if (dwError != 0) {
                fmt::print(fmt::fg(fmt::color::red) | fmt::bg(fmt::color::yellow),
                           "Error opening file dialog, error code: {}\n", dwError);
                std::exit(dwError);
            } else {
                fmt::print(fmt::fg(fmt::color::red) | fmt::bg(fmt::color::yellow),
                           "File selection cancelled by user.\n");
            }
        }

        delete[] szBuffer;
        return selectedFiles;
    }
#endif // _WIN32
};