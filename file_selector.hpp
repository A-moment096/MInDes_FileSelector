#pragma once

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <functional>
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

#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/ostream.h>

namespace fs = std::filesystem;

class FileSelector {
public:
#ifdef __unix__
    FileSelector(const std::string &start, const std::vector<std::string> &exts)
        : currentDirectory(expandTilde(fs::canonical(start))), previousDirectory(currentDirectory),
          filters(exts), cursor(0) {}
#elif defined(_WIN32)
    FileSelector(const std::string &start = ".", const std::vector<std::string> &exts = {})
        : currentDirectory(fs::canonical(start)),
          filters(exts) {}
#endif

    std::vector<std::string> run() {
#ifdef __unix__
        return runLinux();
#endif //__unix__

#ifdef _WIN32
        return runWindows();
#endif // _WIN32
    }

private:
    fs::path currentDirectory;
    std::vector<std::string> filters;
#ifdef __unix__
    // State management
    std::vector<fs::directory_entry> entries;
    size_t cursor;
    std::set<fs::path> selectedPaths;
    termios originalTermios{};
    bool showHelp = false;
    bool inRangeMode;
    bool openFolderinRange;
    bool showFullHelp = false;
    bool showHidden = false;
    bool shouldQuit = false;
    std::string commandBuffer;
    std::string sortPolicy = "type,name";
    bool isSortPolicyChanged = true;
    bool isFiltersChanged = false;
    bool showSelected = true;
    fs::path previousDirectory;
    std::string searchName{};

    std::vector<std::string> runLinux() {
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

    void setTerminalInputMode() {
        termios t;
        tcgetattr(STDIN_FILENO, &t);
        t.c_lflag |= ICANON;
        t.c_lflag |= ECHO;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &t);
    }

    void setTerminalRawMode() {
        termios t;
        tcgetattr(STDIN_FILENO, &t);
        t.c_lflag &= ~(ECHO | ICANON);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &t);
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
        if (!ext.empty()) {
            ext = ext.substr(1);
        }
        return std::find(filters.begin(), filters.end(), ext) != filters.end();
    }

    // Entry processing
    void refreshEntries() {
        bool dirChanged{previousDirectory != currentDirectory};
        bool searchingFile{!searchName.empty()};
        if (isSortPolicyChanged or dirChanged or isFiltersChanged or !searchingFile) {
            if (dirChanged) {
                previousDirectory = currentDirectory;
            }

            entries.clear();
            try {
                for (const auto &entry : fs::directory_iterator(currentDirectory)) {
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

                entrySort();
                isSortPolicyChanged = false;
                isFiltersChanged = false;

            } catch (...) {
            }
        }
    }

    // Entry sorting
    using Entry = std::filesystem::directory_entry;
    using Comparator = std::function<bool(const Entry &, const Entry &)>;

    void entrySort() {
        // Comparator for directories first
        Comparator comparator_directories_first = [](const Entry &a, const Entry &b) {
            return a.is_directory() && !b.is_directory();
        };
        // Comparator for name
        Comparator comparator_alphabetical = [](const Entry &a, const Entry &b) {
            return a.path().filename() < b.path().filename();
        };
        // Comparator for modified time
        Comparator comparator_modified_time = [](const Entry &a, const Entry &b) {
            return fs::last_write_time(a.path()) > fs::last_write_time(b.path());
        };
        // Comparator for extension
        Comparator comparator_extension = [](const Entry &a, const Entry &b) {
            return a.path().extension() < b.path().extension();
        };
        // Comparator for size
        Comparator comparator_size = [](const Entry &a, const Entry &b) {
            auto sizeA = fs::is_regular_file(a) ? fs::file_size(a) : 0;
            auto sizeB = fs::is_regular_file(b) ? fs::file_size(b) : 0;
            return sizeA < sizeB;
        };

        std::unordered_map<std::string, Comparator> comparatorMap = {
            {"dir", comparator_directories_first},
            {"size", comparator_size},
            {"type", comparator_extension},
            {"name", comparator_alphabetical},
            {"time", comparator_modified_time}};

        std::vector<Comparator> sorters;
        std::istringstream ss(sortPolicy);
        std::string token;

        while (std::getline(ss, token, ',')) {
            if (auto it = comparatorMap.find(token); it != comparatorMap.end()) {
                sorters.push_back(it->second);
            } else {
                std::cerr << "Unknown sort key: " << token << std::endl;
            }
        }

        auto finalComparator = combineComparators(sorters);
        std::sort(entries.begin(), entries.end(), finalComparator);
        cursor = std::min(cursor, entries.empty() ? 0 : entries.size() - 1);
    }

    Comparator combineComparators(const std::vector<Comparator> &comps) {
        return [=](const Entry &a, const Entry &b) {
            for (const auto &comp : comps) {
                if (comp(a, b))
                    return true;
                if (comp(b, a))
                    return false;
            }
            return false; // equal
        };
    }

    // Input handling

    void processInput() {
        int key = readKey();
        auto is_colon = [](int c) { return c == ':'; };
        auto is_positive_digit = [](int c) { return c >= '1' && c <= '9'; };

        if (is_colon(key)) { // Command mode
            setTerminalInputMode();
            fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::steel_blue),
                       "Command {}", static_cast<char>(key));
            std::cout.flush();

            commandBuffer = getlineByChar();

            // commandBuffer = static_cast<char>(key) + commandBuffer;
            handleColonCommand();
            setTerminalRawMode();
            (false);
        } else if (is_positive_digit(key)) {
            setTerminalInputMode();
            (true);
            fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::steel_blue),
                       "Number ");
            fmt::print("{}", static_cast<char>(key));
            std::cout.flush();

            commandBuffer = getlineByChar();

            commandBuffer = static_cast<char>(key) + commandBuffer;
            handleSelectionCommand();
            setTerminalRawMode();
            (false);
        } else { // Immediate mode
            handleNormalInput(key);
        }
    }

    std::string getlineByChar() {
        setupTerminal();
        std::string buffer;
        size_t cursor_pos = 0;
        auto moveCmdCursorRight = [&]() {
            std::cout << buffer[cursor_pos] << std::flush;
            ++cursor_pos;
        };
        auto moveCmdCursorLeft = [&]() {
            --cursor_pos;
            std::cout << "\b" << std::flush;
        };
        auto deleteChar = [&]() {
            buffer.erase(cursor_pos - 1, 1);
            --cursor_pos;
            std::cout << "\b\033[s" << buffer.substr(cursor_pos) << ' ' << "\033[u" << std::flush;
        };
        auto deleteCharBack = [&]() {
            buffer.erase(cursor_pos, 1);
            std::cout << "\033[s"; // save cursor pos
            std::cout << buffer.substr(cursor_pos) << ' ';
            std::cout << "\033[u" << std::flush; // restore cursor pos
        };

        int ch;
        while (true) {
            size_t buffer_size = buffer.size();
            ch = readKey();

            switch (ch) {
            case KEY_ESC:
                return "";
                break;
            case KEY_ARROW_LEFT:
                if (cursor_pos > 0) {
                    moveCmdCursorLeft();
                } else {
                    std::cout << "\a" << std::flush;
                }
                break;
            case KEY_ARROW_RIGHT:
                if (cursor_pos < buffer_size) {
                    moveCmdCursorRight();
                } else {
                    std::cout << "\a" << std::flush;
                }
                break;
            case KEY_CTRL_LEFT:
                if (cursor_pos > 0) {
                    moveCmdCursorLeft();
                }
                while (cursor_pos > 0 && isspace(buffer[cursor_pos])) {
                    moveCmdCursorLeft();
                }
                while (cursor_pos > 0 && !isspace(buffer[cursor_pos - 1])) {
                    moveCmdCursorLeft();
                }
                break;
            case KEY_CTRL_RIGHT:
                while (cursor_pos < buffer_size && isspace(buffer[cursor_pos])) {
                    moveCmdCursorRight();
                }
                while (cursor_pos < buffer_size && !isspace(buffer[cursor_pos])) {
                    moveCmdCursorRight();
                }
                break;
            case KEY_HOME:
                while (cursor_pos > 0) {
                    moveCmdCursorLeft();
                }
                break;
            case KEY_END:
                while (cursor_pos < buffer_size) {
                    moveCmdCursorRight();
                }
                break;
            case KEY_DELETE:
                if (cursor_pos < buffer_size) {
                    buffer.erase(cursor_pos, 1);
                    std::cout << "\033[s"; // save cursor pos
                    std::cout << buffer.substr(cursor_pos) << ' ';
                    std::cout << "\033[u" << std::flush; // restore cursor pos
                }
                break;
            case KEY_BACKSPACE:
                if (cursor_pos > 0) {
                    deleteChar();
                } else {
                    std::cout << "\a" << std::flush;
                }
                break;
            case KEY_DELETE_WORD:
                if (cursor_pos > 0) {
                    deleteChar();
                }
                while (cursor_pos > 0 && isspace(buffer[cursor_pos])) {
                    deleteChar();
                }
                while (cursor_pos > 0 && !isspace(buffer[cursor_pos - 1])) {
                    deleteChar();
                }
                break;
            case KEY_DELETE_WORD_BACK:
                while (cursor_pos < buffer_size && isspace(buffer[cursor_pos])) {
                    deleteCharBack();
                }
                while (cursor_pos < buffer_size && !isspace(buffer[cursor_pos])) {
                    deleteCharBack();
                }
                break;
            case KEY_DELETE_LINE:
                while (cursor_pos > 0) {
                    deleteChar();
                }
                break;
            case KEY_ENTER:
                return buffer;
                break;
            default:
                if (std::isprint(ch)) {
                    buffer.insert(cursor_pos, 1, static_cast<char>(ch));
                    std::cout << "\033[s" << buffer.substr(cursor_pos) << "\033[u" << static_cast<char>(ch) << std::flush;
                    ++cursor_pos;
                }
            }
        }
        restoreTerminal();
        return buffer;
    }

    enum Key {
        KEY_NULL = 0,
        KEY_ESC = 27,
        KEY_ENTER = '\r',
        KEY_BACKSPACE = 127,

        KEY_ARROW_LEFT = 1000,
        KEY_ARROW_RIGHT,
        KEY_ARROW_UP,
        KEY_ARROW_DOWN,
        KEY_HOME,
        KEY_END,
        KEY_CTRL_LEFT,
        KEY_CTRL_RIGHT,

        KEY_DELETE,
        KEY_DELETE_WORD,
        KEY_DELETE_WORD_BACK,
        KEY_DELETE_LINE
        // Add more as needed
    };

    int readKey() {
        char seq[10]{};
        size_t readedLength = read(STDIN_FILENO, &seq[0], 10);
        if (readedLength != 1) {
            if (readedLength == 2) {
                switch (seq[1]) {
                case '\177':
                    return KEY_DELETE_WORD;
                case 'd':
                    return KEY_DELETE_WORD_BACK;
                }
            }
            if (readedLength == 3) {
                if (seq[1] == '[') {
                    switch (seq[2]) {
                    case 'A':
                        return KEY_ARROW_UP;
                    case 'B':
                        return KEY_ARROW_DOWN;
                    case 'C':
                        return KEY_ARROW_RIGHT;
                    case 'D':
                        return KEY_ARROW_LEFT;
                    case 'H':
                        return KEY_HOME;
                    case 'F':
                        return KEY_END;
                    }
                }
            } else if (readedLength == 4) {
                switch (seq[2]) {
                case '3':
                    switch (seq[3]) {
                    case '~':
                        return KEY_DELETE;
                    }
                }
            } else if (readedLength == 6) {
                switch (seq[5]) {
                case 'C':
                    return KEY_CTRL_RIGHT;
                case 'D':
                    return KEY_CTRL_LEFT;
                }
            }
            return KEY_NULL;
        } else {
            switch (seq[0]) {
            case '\x1B':
                return KEY_ESC;
            case '\n':
            case '\r':
                return KEY_ENTER;
            case 23:
                return KEY_DELETE_WORD;
            case 21:
                return KEY_DELETE_LINE;
            default:
                return seq[0]; // Normal character
                break;
            }
        }

        return KEY_ESC;
    }

    void handleNormalInput(int key) {
        switch (key) {

        case KEY_ARROW_LEFT:
            navigateParent();
            break;
        case KEY_ARROW_RIGHT:
            toggleSelection_navigateChild();
            break;
        case KEY_ARROW_UP:
            moveCursor(-1);
            break;
        case KEY_ARROW_DOWN:
            moveCursor(1);
            break;
        case 'q':
        case '\n':
        case '\r':
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
        case '!':
            showHelp = !showHelp;
            break;
        case '?':
            showFullHelp = true;
            break;
        case 'H':
            showHidden = !showHidden;
            break;
        case 'S':
            showSelected = !showSelected;
            break;
        default:
            std::cout << "Invalid key pressed: " << key << "\n";
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
        currentDirectory = currentDirectory.parent_path();
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
            currentDirectory = fs::canonical(entry.path());
            cursor = 0;
        }
    }

    void confirmSelection() {
        shouldQuit = true;
    }

    // Command processing

    void handleColonCommand() {

        if (commandBuffer == "q") {
            shouldQuit = true;
        } else if (commandBuffer.find("filter ") != std::string::npos) {
            handleColonConfigFilter();
        } else if (commandBuffer == "help") {
            showFullHelp = true;
        } else if (commandBuffer.find("sort ") != std::string::npos) {
            handleColonSetSortPolicy();
        } else if (commandBuffer.find("search ") != std::string::npos) {
            handleColonSearch();
        } else {
            handleColonPathCommand();
        }
    }

    void handleColonSearch() {
        searchName = commandBuffer.substr(std::string("search ").size());
        if (searchName == "-c") {
            searchName.clear();
            return;
        }
        std::transform(searchName.begin(), searchName.end(), searchName.begin(), ::tolower);
        std::vector<fs::directory_entry> search_results{};
        for (const auto &entry : entries) {
            std::string entry_filename = entry.path().filename();
            std::transform(entry_filename.begin(), entry_filename.end(), entry_filename.begin(), ::tolower);
            if (entry_filename.find(searchName) != std::string::npos) {
                search_results.push_back(entry);
            }
        }
        entries = search_results;
    }

    void handleColonConfigFilter() {
        std::string exts = commandBuffer.substr(std::string("filter ").size());
        if (exts == "-c") {
            filters.clear();
            return;
        }
        filters = split_multi_delim(exts, " ,");
        isFiltersChanged = true;
    }

    void handleColonSetSortPolicy() {
        sortPolicy = commandBuffer.substr(std::string("sort ").size());
        isSortPolicyChanged = true;
    }

    void handleColonPathCommand() {
        try {
            fs::path newPath = expandTilde(commandBuffer.substr(1));
            if (fs::is_directory(newPath)) {
                currentDirectory = fs::canonical(newPath);
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
                currentDirectory = fs::canonical(entry.path());
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
        const auto hidden_style = showHidden ? fg(fmt::color::light_green) : fg(fmt::color::light_pink);
        const auto search_style = fmt::emphasis::italic | fg(fmt::color::sea_green);

        if (showHelp) {
            printQuickHelp();
        } else {
            fmt::print(fg(fmt::color::dark_gray) | bg(fmt::color::light_gray), "Press '!' for floating help or '?' for full features");
        }
        fmt::print("\n");
        fmt::print(header_style, "📁 {}\n", currentDirectory.string());

        if (!searchName.empty()) {
            std::string searchPrompt = fmt::format(fg(fmt::color::sea_green), "[Searching: ");
            searchPrompt += fmt::format(search_style, "{}", searchName);
            searchPrompt += fmt::format(fg(fmt::color::sea_green), "] ");
            std::cout << searchPrompt;
            if (previousDirectory != currentDirectory) {
                searchName.clear();
            }
        }

        std::string filters_string;
        if (filters.empty()) {
            filters_string = fmt::format(fg(fmt::color::gray), "NONE");
        } else {
            for (const auto &filter : filters) {
                filters_string += filter;
                filters_string += ", ";
            }
            filters_string = filters_string.substr(0, filters_string.size() - 2);
        }
        fmt::print(fg(fmt::color::aqua), "[Applied Filter: {}", filters_string);
        fmt::print(fg(fmt::color::aqua), "] ");

        fmt::print(hidden_style, "[Show Hidden?: {}] ", showHidden ? "YES" : "No");

        fmt::print("\n");
    }

    void drawFileList() {
        const auto dir_style = fg(fmt::color::deep_sky_blue);
        const auto file_style = fg(fmt::color::white);
        const auto no_permission_style = fg(fmt::color::red);
        const auto time_style = fg(fmt::color::pale_golden_rod);
        const auto size_style = fg(fmt::color::royal_blue);
        std::string item_bar;
        item_bar = fmt::format(file_style, "{:<7}  {}  {:<40}", "", "No", "File Name");
        item_bar += fmt::format(time_style, " {:<12}", "Modify Time", "Size");
        item_bar += fmt::format(size_style, "  {}\n", "Size");
        fmt::print("{}", item_bar);

        for (size_t i = 0; i < entries.size(); ++i) {
            bool hasPermission = true;
            const auto &entry = entries[i];
            try {
                (void)fs::canonical(entry.path());
            } catch (...) {
                hasPermission = false;
            }
            bool selected = false;
            if (hasPermission) {
                selected = selectedPaths.count(fs::canonical(entry.path()));
            } else {
                selected = false;
            }

            std::string entry_line;

            auto selectedPrompt = [selected, hasPermission]() -> std::string {
                if (selected) {
                    return "[✓] ";
                } else if (!hasPermission) {
                    return "[✗] ";
                } else {
                    return "[ ] ";
                }
            };

            entry_line += i == cursor ? "▶ " : "  ";
            entry_line += selectedPrompt();

            auto print_style = dir_style;

            bool dir_entry = entry.is_directory();
            if (hasPermission) {
                if (dir_entry) {
                    print_style = dir_style;
                    entry_line += fmt::format(print_style | fmt::emphasis::bold, "📁 ");

                } else {
                    print_style = file_style;
                    entry_line += fmt::format(print_style, "📄 ");
                }
            } else {
                print_style = no_permission_style;
                entry_line += fmt::format(print_style, "❌ ");
            }

            entry_line += fmt::format(print_style, "{:2}  {:<40} ",
                                      i + 1, entry.path().filename().string());

            try {
                auto file_time = fs::last_write_time(entry);
                std::time_t tt = to_time_t(file_time);

                std::tm local_tm = *std::localtime(&tt);
                std::time_t now_tt = std::time(nullptr);
                std::tm now_tm = *std::localtime(&now_tt);
                constexpr std::time_t six_months_sec = 60 * 60 * 24 * 30 * 6;

                if (std::abs(now_tt - tt) > six_months_sec) {
                    // Show year
                    entry_line += fmt::format(time_style, "{:%b %e  %Y}  ", local_tm);
                } else {
                    // Show time
                    entry_line += fmt::format(time_style, "{:%b %e %H:%M}  ", local_tm);
                }
            } catch (...) {
            }
            try {
                constexpr const char *suffixes[] = {"B", "K", "M", "G", "T", "P"};
                int choose_suffix = 0;
                double count;
                if (!dir_entry)
                    count = static_cast<double>(entry.file_size());

                while (count >= 1024 && choose_suffix < 5) {
                    count /= 1024;
                    ++choose_suffix;
                }
                // fmt::print("{}",entry.file_size());
                if (!dir_entry) {
                    if (choose_suffix == 0) {
                        entry_line += fmt::format(size_style, "{}{}", static_cast<std::uintmax_t>(count), suffixes[choose_suffix]);
                    } else {
                        entry_line += fmt::format(size_style, "{:.1f}{}", count, suffixes[choose_suffix]);
                    }
                } else {
                    entry_line += fmt::format(size_style, " - ");
                }
            } catch (...) {
            }
            fmt::print("{}\n", entry_line);
        }
    }

    void drawFooter() {
        if (openFolderinRange) {
            std::cout << "Can't open a directory in range mode\n";
            openFolderinRange = false;
        }
        std::cout << "\nSelected: " << selectedPaths.size() << " files\n";
        if (showSelected) {
            for (auto &f : selectedPaths)
                std::cout << " - " << f.filename() << "\n";
        }
    }

    std::time_t to_time_t(fs::file_time_type file_time) {
        // Convert file_time_type to system_clock::time_point
        using namespace std::chrono;
        auto sctp = time_point_cast<system_clock::duration>(
            file_time - fs::file_time_type::clock::now() + system_clock::now());
        return system_clock::to_time_t(sctp);
    }

    void printFullHelp() {
        const auto title_style = fmt::emphasis::bold | fg(fmt::color::gold);
        const auto section_style = fg(fmt::color::aqua) | fmt::emphasis::underline;
        const auto warn_style = fg(fmt::color::orange) | fmt::emphasis::bold;

        fmt::print(title_style, "\n{:-^80}\n", " HELP ");
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

        fmt::print(title_style, "\n{:-^80}\n", "");
    }

    void printQuickHelp() {
        const auto title_style = fmt::emphasis::bold | fg(fmt::color::gold);
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

    std::vector<std::string> split_multi_delim(const std::string &input, const std::string &delims) {
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
#endif

#ifdef _WIN32
    std::string windowTitle{};

    std::vector<std::string> runWindows() {
        // Allocate large buffer to hold multiple file names
        const DWORD bufferSize = 65536; // 64 KB
        char *szBuffer = new char[bufferSize];
        ZeroMemory(szBuffer, bufferSize);

        OPENFILENAMEA ofn = {0};
        ofn.lStructSize = sizeof(ofn);
        auto initial_directory_path = currentDirectory.string().c_str();
        ofn.lpstrInitialDir = initial_directory_path;

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