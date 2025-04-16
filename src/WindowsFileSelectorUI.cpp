#ifdef _WIN32

#include "WindowsFileSelectorUI.hpp"

#include <algorithm>
#include <windows.h>

#include <fmt/color.h>
#include <fmt/core.h>

WindowsFileSelectorUI::WindowsFileSelectorUI(const std::string &start, const std::vector<std::string> &exts)
    : startPath(start), extensions(exts) {
}

std::vector<std::string> WindowsFileSelectorUI::run() {
    // Allocate large buffer to hold multiple file names
    const DWORD bufferSize = 65536; // 64 KB
    char *szBuffer = new char[bufferSize];
    ZeroMemory(szBuffer, bufferSize);

    OPENFILENAMEA ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    auto initial_directory_path = startPath.c_str();
    ofn.lpstrInitialDir = initial_directory_path;

    std::string filter{};
    for (const auto &ext : extensions) {
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