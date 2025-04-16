// TerminalManager.hpp
#pragma once
#ifdef __unix__
#include <string>
#include <termios.h>
#include <unistd.h>
#include <vector>

class TerminalManager {
public:
    TerminalManager();
    ~TerminalManager();
    void setRawMode();
    void setCanonicalMode();
    void restoreTerminal();
    int readKey(); // mapping of raw key input to our enum keys
    std::string getLineByChar();
    // (Windows version will use a different approach and may be handled elsewhere)
private:
    termios originalTermios{};
    std::vector<std::string> commandHistory{};
    unsigned int historyPosition{0};
    void writeBuffer(std::string &buffer, size_t &cursor_pos, int ch);
};
#endif // __unix__