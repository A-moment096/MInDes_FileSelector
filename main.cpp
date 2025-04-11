#include "file_selector.hpp"

int main() {
    FileSelector nav(".", {".mindes"});
    auto files = nav.run();

    std::cout << "\nSelected:\n";
    for (auto& f : files)
        std::cout << " - " << f << "\n";

    return 0;
}
