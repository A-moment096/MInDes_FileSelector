#include "FileSelector.hpp"
#include "IFileSelectorUI.hpp"

#ifdef __unix__
#include "UnixFileSelectorUI.hpp"
#elif defined(_WIN32)
#include "WindowsFileSelectorUI.hpp"
#endif

class FileSelector::Impl {
public:
    Impl(const std::string &start, const std::vector<std::string> &exts) {
#ifdef __unix__
        ui = std::make_unique<UnixFileSelectorUI>(start, exts);
#elif defined(_WIN32)
        ui = std::make_unique<WindowsFileSelectorUI>(start, exts);
#endif
    }

    std::vector<std::string> run() {
        return ui->run();
    }

private:
    std::unique_ptr<IFileSelectorUI> ui;
};

FileSelector::FileSelector(const std::string &start, const std::vector<std::string> &exts)
    : pImpl(std::make_unique<Impl>(start, exts)) {}

FileSelector::~FileSelector() = default;

std::vector<std::string> FileSelector::run() {
    return pImpl->run();
}
