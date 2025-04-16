#include "UIRenderer.hpp"
#ifdef __unix__
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
UIRenderer::UIRenderer() {
}

void UIRenderer::drawHeader(const std::string &currentDirectory,
                            const std::vector<std::string> &activeFilters,
                            bool isShowHidden,
                            const std::string &searchName,
                            bool isShowHint, bool isShowSelected) {

    constexpr const auto header_style = fmt::emphasis::bold | fg(fmt::color::light_blue);
    constexpr const auto search_style = fmt::emphasis::italic | fg(fmt::color::sea_green);
    const auto hidden_style = isShowHidden ? fg(fmt::color::light_green) : fg(fmt::color::light_pink);
    const auto selected_style = isShowSelected ? fg(fmt::color::light_green) : fg(fmt::color::light_pink);

    if (isShowHint) {
        printQuickHelp();
    } else {
        fmt::print(fg(fmt::color::dark_gray) | bg(fmt::color::light_gray), "Press '!' for floating help or '?' for full features\n");
    }
    fmt::print(header_style, "📁 {}\n", currentDirectory);

    std::string status_bar_1{};
    std::string status_bar_2{};
    if (!searchName.empty()) {
        status_bar_1 += getSearchStatus(searchName);
    }
    status_bar_1 += getFilterStatus(activeFilters);
    status_bar_2 += fmt::format(hidden_style, "[Show Hidden? : {}] ", isShowHidden ? "YES" : "N0");
    status_bar_2 += fmt::format(selected_style, "[Show Selected? : {}] ", isShowSelected ? "YES" : "NO");
    fmt::print("{}\n", status_bar_1);
    fmt::print("{}\n", status_bar_2);
}

void UIRenderer::drawFileList(const std::vector<fs::directory_entry> &entries,
                              size_t cursor,
                              const std::set<fs::path> &selectedPaths) {
    constexpr const auto dir_style = fg(fmt::color::deep_sky_blue);
    constexpr const auto file_style = fg(fmt::color::white);
    constexpr const auto no_permission_style = fg(fmt::color::red);
    constexpr const auto time_style = fg(fmt::color::pale_golden_rod);
    constexpr const auto size_style = fg(fmt::color::royal_blue);
    constexpr const auto type_style = fg(fmt::color::magenta);

    std::string item_bar;
    item_bar = fmt::format(file_style, "{:<7}  {}  {:<40}", "", "No", "File Name");
    item_bar += fmt::format(type_style, " {:<7}", "Type");
    item_bar += fmt::format(time_style, " {:<12}", "Modify Time", "Size");
    item_bar += fmt::format(size_style, "  {}", "Size");
    fmt::print("{}\n", item_bar);

    for (size_t i = 0; i < entries.size(); ++i) {
        bool has_permission = true;
        const auto &entry = entries[i];
        bool is_selected = false;

        try {
            is_selected = selectedPaths.count(fs::canonical(entry.path()));
        } catch (...) {
            has_permission = false;
        }

        auto selectedCheckBox = [is_selected, has_permission]() -> std::string {
            if (has_permission) {
                if (is_selected) {
                    return "[✓] ";
                } else {
                    return "[ ] ";
                }
            } else {
                return "[✗] ";
            }
        };
        auto getFormattedFileExtn = [type_style](const fs::directory_entry &entry) -> std::string {
            constexpr const auto type_style = fg(fmt::color::magenta);
            std::string formatted_extension;
            if (entry.is_directory()) {
                formatted_extension = fmt::format(type_style, "{:<7.{}s} ", "DIR", 7);
            } else {
                formatted_extension = entry.path().extension().string();
                formatted_extension = formatted_extension.substr(1);
                std::transform(formatted_extension.begin(), formatted_extension.end(), formatted_extension.begin(), ::toupper);
                formatted_extension = fmt::format(type_style, "{:<7.{}s} ", formatted_extension, 7);
            }
            return formatted_extension;
        };

        std::string entry_line;
        try {
            entry_line += i == cursor ? "▶ " : "  ";
            entry_line += selectedCheckBox();
            entry_line += getFormattedFileName(entry, i, has_permission);
            entry_line += getFormattedFileExtn(entry);
            entry_line += getFormattedFileTime(entry);
            entry_line += getFormattedFileSize(entry);
        } catch (...) {
        }

        fmt::print("{}\n", entry_line);
    }
}

void UIRenderer::drawFooter(const std::set<fs::path> &selectedPaths, bool showSelected) {
    fmt::print("\nSelected: {} files\n", selectedPaths.size());
    if (showSelected) {
        for (auto &f : selectedPaths) {
            fmt::print(" - {}\n", f.filename().string());
        }
    }
}

std::string UIRenderer::getFormattedFileName(const fs::directory_entry &entry, size_t number, bool has_permission) {
    constexpr const auto dir_style = fg(fmt::color::deep_sky_blue);
    constexpr const auto file_style = fg(fmt::color::white);
    constexpr const auto no_permission_style = fg(fmt::color::red);

    std::string formatted_name{};
    auto print_style = dir_style;

    bool dir_entry = entry.is_directory();
    if (has_permission) {
        if (dir_entry) {
            print_style = dir_style;
            formatted_name += fmt::format(print_style | fmt::emphasis::bold, "📁 ");

        } else {
            print_style = file_style;
            formatted_name += fmt::format(print_style, "📄 ");
        }
    } else {
        print_style = no_permission_style;
        formatted_name += fmt::format(print_style, "❌ ");
    }

    formatted_name += fmt::format(print_style, "{:2}  {:<40.{}s} ",
                                  number + 1, entry.path().filename().string(), 40);

    return formatted_name;
}

std::string UIRenderer::getFormattedFileTime(const fs::directory_entry &entry) {
    using namespace std::chrono;
    constexpr const auto time_style = fg(fmt::color::pale_golden_rod);

    auto ftime = fs::last_write_time(entry);
    auto system_time = system_clock::time_point(
        duration_cast<system_clock::duration>(ftime.time_since_epoch()));
    const auto now = system_clock::now();
    const auto six_months_ago = now - hours(183 * 24);

    std::string formatted_time{};
    if (system_time > six_months_ago) {
        formatted_time += fmt::format(time_style, "{:%b %e %Y}  ", system_time);
    } else {
        formatted_time += fmt::format(time_style, "{:%b %e %H:%M}  ", system_time);
    }
    return formatted_time;
}

std::string UIRenderer::getFormattedFileSize(const fs::directory_entry &entry) {
    constexpr const auto size_style = fg(fmt::color::royal_blue);
    constexpr const char *suffixes[] = {"B", "K", "M", "G", "T", "P"};

    int choose_suffix = 0;
    double count;
    bool is_dir_entry = entry.is_directory();
    if (!is_dir_entry)
        count = static_cast<double>(entry.file_size());

    while (count >= 1024 && choose_suffix < 5) {
        count /= 1024;
        ++choose_suffix;
    }

    std::string formatted_size{};
    if (!is_dir_entry) {
        if (choose_suffix == 0) {
            formatted_size += fmt::format(size_style, "{}{}", static_cast<std::uintmax_t>(count), suffixes[choose_suffix]);
        } else {
            formatted_size += fmt::format(size_style, "{:.1f}{}", count, suffixes[choose_suffix]);
        }
    } else {
        formatted_size += fmt::format(size_style, "  -  ");
    }
    return formatted_size;
}

std::string UIRenderer::getSearchStatus(const std::string &searchName) {
    std::string searchPrompt = fmt::format(fg(fmt::color::sea_green), "[Searching: ");
    constexpr const auto search_style = fmt::emphasis::italic | fg(fmt::color::sea_green);
    searchPrompt += fmt::format(search_style, "{}", searchName);
    searchPrompt += fmt::format(fg(fmt::color::sea_green), "] ");
    // Clear searchPrompt by caller
    return searchPrompt;
}

std::string UIRenderer::getFilterStatus(const std::vector<std::string> &activeFilters) {
    std::string active_filter_status;
    if (activeFilters.empty()) {
        active_filter_status = fmt::format(fg(fmt::color::gray), "NONE");
    } else {
        for (const auto &filter : activeFilters) {
            active_filter_status += filter;
            if (filter != activeFilters.back()) {
                active_filter_status += ", ";
            }
        }
    }
    active_filter_status = fmt::format(fg(fmt::color::aqua), "[Applied Filter: {}", active_filter_status);
    active_filter_status += fmt::format(fg(fmt::color::aqua), "] ");
    return active_filter_status;
}

void UIRenderer::drawHelp(bool fullHelp) {
    fmt::print("\033[2J\033[H"); // Clear screen
    if (fullHelp) {
        printFullHelp();
        fullHelp = false;
        fmt::print("\033[2J\033[H"); // Clear screen again
    }
}

void UIRenderer::printFullHelp() {

    constexpr const auto title_style = fmt::emphasis::bold | fg(fmt::color::gold);
    constexpr const auto section_style = fg(fmt::color::aqua) | fmt::emphasis::underline;
    constexpr const auto subsection_style = fg(fmt::color::light_sky_blue) | fmt::emphasis::bold;
    constexpr const auto param_style = fg(fmt::color::light_green);
    constexpr const auto example_style = fg(fmt::color::light_gray) | fmt::emphasis::italic;
    constexpr const auto note_style = fg(fmt::color::orange) | fmt::emphasis::italic;
    constexpr const auto warn_style = fg(fmt::color::orange) | fmt::emphasis::bold;

    const std::vector<std::string> contents{
        // Title
        "",
        fmt::format(title_style, "{:-^80}", " HELP "),

        // Navigation Section
        "",
        fmt::format(section_style, "[ Navigation & Movement ]"),
        "",
        fmt::format(subsection_style, "Basic Movement:"),
        fmt::format("  {:<18} {}", "↑/k", "Move cursor up"),
        fmt::format("  {:<18} {}", "↓/j", "Move cursor down"),
        fmt::format("  {:<18} {}", "←/h/Backspace", "Go to parent directory"),
        fmt::format("  {:<18} {}", "→/l/Space", "Enter directory (📁) / Toggle file (📄)"),

        // Selection Section
        "",
        fmt::format(section_style, "[ Selection Modes ]"),
        "",
        fmt::format("  {:<18} Press number to start selection mode", "Activation:"),
        fmt::format("  {:<18} Confirm with <Enter>, cancel with <ESC>", "Completion:"),
        "",
        // fmt::format(subsection_style, "Number Mode (1-9):"),
        fmt::format("  {:<18} Select files using:", "Usage:"),
        fmt::format("    {:<16} - Single file (e.g., '3')", ""),
        fmt::format("    {:<16} - Ranges (e.g., '1-5')", ""),
        fmt::format("    {:<16} - Combinations (e.g., '1-3,5,7')", ""),
        fmt::format(warn_style, "  {}", "Note: Directories cannot be multi-selected"),

        "",
        fmt::format(section_style, "[ Command Mode (:) ]"),
        "",
        fmt::format("  {:<18} Press <:> to start command mode", "Activation:"),
        fmt::format("  {:<18} Confirm with <Enter>, cancel with <ESC>", "Completion:"),
        "",
        fmt::format(subsection_style, "Path Navigation:"),
        fmt::format("  {:<18} {}", ":<path>",
                    "Jump to specified filesystem path"),
        fmt::format(param_style, "  {:<18} {}", "  Parameters:",
                    "Absolute path or relative path from current directory"),
        fmt::format(example_style, "  {:<18} {}", "  Example:",
                    ":~/documents  :../parent_dir  :/usr/local"),

        "",
        fmt::format(subsection_style, "Filter Operations:"),
        fmt::format("  {:<18} {}", ":filter <extensions>",
                    "Show files with specified extensions"),
        fmt::format(param_style, "  {:<18} {}", "  Parameters:",
                    "Comma/space-separated list of extensions"),
        fmt::format(example_style, "  {:<18} {}", "  Example:",
                    ":filter txt,cpp pdf  :filter "),
        fmt::format(note_style, "  {:<18} {}", "  Note:",
                    "Empty filter resets to show all file types"),

        "",
        fmt::format(subsection_style, "Search Operations:"),
        fmt::format("  {:<18} {}", ":search <pattern>",
                    "Search files by name/content"),
        fmt::format(param_style, "  {:<18} {}", "  Parameters:",
                    "Search string (case-insensitive)"),
        fmt::format(example_style, "  {:<18} {}", "  Example:",
                    ":search report2023  :search "),
        fmt::format(note_style, "  {:<18} {}", "  Note:",
                    "Empty search resets filtering"),
        fmt::format(note_style, "  {:<18} {}", "  ",
                    "Space will be part of searched name"),

        "",
        fmt::format(subsection_style, "Sort Operations:"),
        fmt::format("  {:<18} {}", ":sort <criteria>",
                    "Set sorting criteria hierarchy"),
        fmt::format(param_style, "  {:<18} {}", "  Parameters:",
                    "Comma/space-separated combination of:"),
        fmt::format("    {:<16} {}", "", "dir   - Directories first"),
        fmt::format("    {:<16} {}", "", "type  - File extension"),
        fmt::format("    {:<16} {}", "", "name  - Alphabetical order"),
        fmt::format("    {:<16} {}", "", "time  - Modification time"),
        fmt::format("    {:<16} {}", "", "size  - File size"),
        fmt::format(example_style, "  {:<18} {}", "  Example:",
                    ":sort dir,name  :sort time"),

        "",
        fmt::format(subsection_style, "Display Settings:"),
        fmt::format("  {:<18} {}", "H",
                    "Toggle hidden files visibility"),
        fmt::format("  {:<18} {}", "S",
                    "Toggle selected files visibility"),

        "",
        fmt::format(subsection_style, "Other Commands:"),
        fmt::format("  {:<18} {}", ":Q",
                    "Finish file selection"),
        fmt::format("  {:<18} {}", ":help",
                    "Open this help"),
        // Program Operations
        "",
        fmt::format(section_style, "[ Program Operations ]"),
        fmt::format("  {:<18} {}", "q", "Finish file selection"),
        fmt::format("  {:<18} {}", "!", "Toggle quick help"),
        fmt::format("  {:<18} {}", "?", "Show full help"),

        // Footer
        fmt::format(title_style, "{:-^80}", "")};

    FILE *pipe = popen("less -R", "w");
    if (!pipe) {
        throw std::runtime_error("Failed to open pipe to less.");
    }

    // Send output to less
    for (const auto &line : contents) {
        fprintf(pipe, "%s\n", line.c_str());
    }

    pclose(pipe);
}
void UIRenderer::printQuickHelp() {
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
#endif // __unix__