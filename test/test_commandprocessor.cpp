// test_commandprocessor.cpp
#define CATCH_CONFIG_MAIN
#include "CommandProcessor.hpp"
#include "FileSystemManager.hpp"
#include "UIRenderer.hpp"
#include "catch_amalgamated.hpp"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

// Create stubs/mocks if necessary, here a dummy UIRenderer with minimal implementation.
class DummyUIRenderer : public UIRenderer {
public:
    void drawHelp(bool fullHelp) override {
        // Minimal implementation for testing.
    }
};

TEST_CASE("Test CommandProcessor navigation", "[CommandProcessor]") {
    fs::path tempDir = fs::temp_directory_path() / "test_cmdproc_nav";
    fs::create_directories(tempDir);

    // Create dummy entries.
    {
        std::ofstream(tempDir / "file1.txt") << "dummy";
        std::ofstream(tempDir / "file2.txt") << "dummy";
    }

    FileSystemManager fsManager(tempDir, {"txt"});
    fsManager.refreshDirectory(true);
    DummyUIRenderer uiRenderer;
    CommandProcessor cmdProc(fsManager, uiRenderer);

    // Initial cursor should be at 0.
    REQUIRE(cmdProc.getCursor() == 0);

    // Simulate pressing the down arrow.
    cmdProc.processImmediateInput(1003);
    REQUIRE(cmdProc.getCursor() == 1);

    // Simulate pressing quit.
    cmdProc.processImmediateInput('q');
    REQUIRE(cmdProc.shouldQuit() == true);

    std::set<fs::path> empty_set{};
    uiRenderer.drawFileList(fsManager.getEntries(), 0, empty_set);
    // Clean up.
    fs::remove_all(tempDir);
}
