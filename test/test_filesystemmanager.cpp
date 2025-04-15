// test_filesystemmanager.cpp
#define CATCH_CONFIG_MAIN
#include "FileSystemManager.hpp"
#include "catch_amalgamated.hpp"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

TEST_CASE("Test tilde expansion", "[FileSystemManager]") {
    // Assume HOME is set to some non-empty value.
    fs::path p = FileSystemManager::expandTilde("~/catch2test");
    // The expanded path should not contain '~'
    REQUIRE(p.string().find("~") == std::string::npos);
}

TEST_CASE("Test directory refresh and filtering", "[FileSystemManager]") {
    // Create a temporary directory structure for testing.
    fs::path tempDir = fs::temp_directory_path() / "test_fs_manager";
    fs::create_directories(tempDir);

    // Create dummy files
    fs::path file1 = tempDir / "file1.txt";
    fs::path file2 = tempDir / "file2.cpp";
    fs::path hiddenFile = tempDir / ".hidden.txt";
    {
        std::ofstream(file1) << "test";
        std::ofstream(file2) << "test";
        std::ofstream(hiddenFile) << "test";
    }

    // Initialize with a filter for txt files.
    FileSystemManager fsManager(tempDir, {"txt"});

    // Refresh without showing hidden files.
    fsManager.refreshDirectory(false);
    auto entries = fsManager.getEntries();
    // Should include file1.txt but not file2.cpp or the hidden file.
    bool foundFile1 = false, foundFile2 = false, foundHidden = false;
    for (const auto &e : entries) {
        auto name = e.path().filename().string();
        if (name == "file1.txt")
            foundFile1 = true;
        if (name == "file2.cpp")
            foundFile2 = true;
        if (name == ".hidden.txt")
            foundHidden = true;
    }
    REQUIRE(foundFile1 == true);
    REQUIRE(foundFile2 == false);
    REQUIRE(foundHidden == false);

    // Clean up
    fs::remove(file1);
    fs::remove(file2);
    fs::remove(hiddenFile);
    fs::remove(tempDir);
}

TEST_CASE("Test search functionality", "[FileSystemManager]") {
    fs::path tempDir = fs::temp_directory_path() / "test_fs_manager_search";
    fs::create_directories(tempDir);

    // Create files with known names.
    fs::path fileA = tempDir / "apple.txt";
    fs::path fileB = tempDir / "banana.txt";
    {
        std::ofstream(fileA) << "test";
        std::ofstream(fileB) << "test";
    }

    FileSystemManager fsManager(tempDir, {"txt"});
    fsManager.refreshDirectory(true); // show all including hidden (if any)
    fsManager.search("app");
    auto entries = fsManager.getEntries();
    // Only the file with "apple" should be returned.
    REQUIRE(entries.size() == 1);
    REQUIRE(entries.front().path().filename() == "apple.txt");

    // Clean up
    fs::remove(fileA);
    fs::remove(fileB);
    fs::remove(tempDir);
}

TEST_CASE("Test empty filter", "[FileSystemManager]") {
    fs::path tempDir = fs::temp_directory_path() / "test_fs_manager_search";
    fs::create_directories(tempDir);

    // Create files with known names.
    fs::path fileA = tempDir / "apple.txt";
    fs::path fileB = tempDir / "banana.abc";
    fs::path fileC = tempDir / ".banana.mkl";
    fs::path dirTest = tempDir / "test_dir";
    fs::create_directories(dirTest);
    {
        std::ofstream(fileA) << "test";
        std::ofstream(fileB) << "test";
        std::ofstream(fileC) << "test";
    }

    FileSystemManager fsManager(tempDir, {});
    fsManager.refreshDirectory(true); // show all including hidden (if any)
    auto entries = fsManager.getEntries();
    // Only the file with "apple" should be returned.
    REQUIRE(entries.size() == 4);

    // Clean up
    fs::remove(fileA);
    fs::remove(fileB);
    fs::remove(fileC);
    fs::remove(dirTest);
    fs::remove(tempDir);
}
