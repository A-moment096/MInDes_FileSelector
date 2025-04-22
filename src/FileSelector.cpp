#include "FileSelector.hpp"
#include "IFileSelectorUI.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#ifdef __unix__
#include "UnixFileSelectorUI.hpp"
#elif defined(_WIN32)
#include "WindowsFileSelectorUI.hpp"
#endif

class FileSelector::Impl {
public:
    Impl(const fs::path &start, const std::vector<std::string> &exts) {
#ifdef __unix__
        ui = std::make_unique<UnixFileSelectorUI>(start, exts);
#elif defined(_WIN32)
        ui = std::make_unique<WindowsFileSelectorUI>(start, exts);
#endif
    }

    std::vector<fs::path> run() {
        return ui->run();
    }

private:
    std::unique_ptr<IFileSelectorUI> ui;
};

FileSelector::FileSelector(const fs::path &start, const std::vector<std::string> &exts)
    : pImpl(std::make_unique<Impl>(start, exts)) {}

FileSelector::~FileSelector() = default;

std::vector<fs::path> FileSelector::run() {
    return pImpl->run();
}
