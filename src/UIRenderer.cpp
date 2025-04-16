#include "UIRenderer.hpp"
#ifdef __unix__
UIRenderer::UIRenderer() {
}

void UIRenderer::drawHeader(const std::string &currentDirectory,
                            const std::vector<std::string> &activeFilters,
                            bool showHidden,
                            const std::string &searchName,
                            bool isShowHint) {

    constexpr const auto header_style = fmt::emphasis::bold | fg(fmt::color::light_blue);
    constexpr const auto search_style = fmt::emphasis::italic | fg(fmt::color::sea_green);
    const auto hidden_style = showHidden ? fg(fmt::color::light_green) : fg(fmt::color::light_pink);

    if (isShowHint) {
        printQuickHelp();
    } else {
        fmt::print(fg(fmt::color::dark_gray) | bg(fmt::color::light_gray), "Press '!' for floating help or '?' for full features\n");
    }
    fmt::print(header_style, "📁 {}\n", currentDirectory);

    std::string status_bar{};
    if (!searchName.empty()) {
        status_bar += getSearchStatus(searchName);
    }
    status_bar += getFilterStatus(activeFilters);
    status_bar += fmt::format(hidden_style, "[Show Hidden?: {}] ", showHidden ? "YES" : "No");
    fmt::print("{}\n", status_bar);
}

void UIRenderer::drawFileList(const std::vector<fs::directory_entry> &entries,
                              size_t cursor,
                              const std::set<fs::path> &selectedPaths) {
    constexpr const auto dir_style = fg(fmt::color::deep_sky_blue);
    constexpr const auto file_style = fg(fmt::color::white);
    constexpr const auto no_permission_style = fg(fmt::color::red);
    constexpr const auto time_style = fg(fmt::color::pale_golden_rod);
    constexpr const auto size_style = fg(fmt::color::royal_blue);

    std::string item_bar;
    item_bar = fmt::format(file_style, "{:<7}  {}  {:<40}", "", "No", "File Name");
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

        std::string entry_line;
        try {
            entry_line += i == cursor ? "▶ " : "  ";
            entry_line += selectedCheckBox();
            entry_line += getFormattedFileName(entry, i, has_permission);
            entry_line += getFormattedFileTime(entry);
            entry_line += getFormattedFileSize(entry);
        } catch (...) {
        }

        fmt::print("{}\n", entry_line);
    }
}

void UIRenderer::drawFooter(const std::set<fs::path> &selectedPaths, bool showSelected) {
    // if (openFolderinRange) {
    //     std::cout << "Can't open a directory in range mode\n";
    //     openFolderinRange = false;
    // }
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

    formatted_name += fmt::format(print_style, "{:2}  {:<40} ",
                                  number + 1, entry.path().filename().string());

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
        formatted_size += fmt::format(size_style, " - ");
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
        fmt::print(fg(fmt::color::steel_blue) | fmt::emphasis::bold,
                   "\nPress any key to continue...\n");
        (void)getchar();
        fmt::print("\033[2J\033[H"); // Clear screen again
    }
}

void UIRenderer::printFullHelp() {
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