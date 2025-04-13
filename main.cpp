#include "file_selector.hpp"

int main() {
    FileSelector nav(".", {});
    auto files = nav.run();

    std::cout << "\nSelected:\n";
    for (auto& f : files)
        std::cout << " - " << f << "\n";

    return 0;
}
