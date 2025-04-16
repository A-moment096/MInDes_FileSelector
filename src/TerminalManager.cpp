// TerminalManager.cpp
#ifdef __unix__
#include "TerminalManager.hpp"
#include "KeyEnum.hpp"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <unistd.h>

TerminalManager::TerminalManager() {
    tcgetattr(STDIN_FILENO, &originalTermios);
    setRawMode();
}
TerminalManager::~TerminalManager() {
    restoreTerminal();
}

void TerminalManager::setRawMode() {
    termios raw = originalTermios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void TerminalManager::setCanonicalMode() {
    termios canonical = originalTermios;
    canonical.c_lflag |= (ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &canonical);
}

void TerminalManager::restoreTerminal() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalTermios);
}

// Implement readKey() here (using your existing logic to decipher escape sequences)
int TerminalManager::readKey() {
    char seq[10]{};
    size_t readedLength = read(STDIN_FILENO, &seq[0], 10);
    if (readedLength != 1) {
        if (readedLength == 2) {
            switch (seq[1]) {
            case '\177':
                return KEY_DELETE_WORD;
            case 'd':
                return KEY_DELETE_WORD_BACK;
            }
        }
        if (readedLength == 3) {
            if (seq[1] == '[') {
                switch (seq[2]) {
                case 'A':
                    return KEY_ARROW_UP;
                case 'B':
                    return KEY_ARROW_DOWN;
                case 'C':
                    return KEY_ARROW_RIGHT;
                case 'D':
                    return KEY_ARROW_LEFT;
                case 'H':
                    return KEY_HOME;
                case 'F':
                    return KEY_END;
                }
            }
        } else if (readedLength == 4) {
            switch (seq[2]) {
            case '3':
                switch (seq[3]) {
                case '~':
                    return KEY_DELETE;
                }
            }
        } else if (readedLength == 6) {
            switch (seq[5]) {
            case 'C':
                return KEY_CTRL_RIGHT;
            case 'D':
                return KEY_CTRL_LEFT;
            }
        }
        return KEY_NULL;
    } else {
        switch (seq[0]) {
        case '\x1B':
            return KEY_ESC;
        case '\n':
        case '\r':
            return KEY_ENTER;
        case 23:
            return KEY_DELETE_WORD;
        case 21:
            return KEY_DELETE_LINE;
        default:
            return seq[0]; // Normal character
            break;
        }
    }

    return KEY_ESC;
}
std::string TerminalManager::getLineByChar() {
    setRawMode();
    std::string buffer;
    size_t cursor_pos = 0;
    auto moveCmdCursorRight = [&]() {
        std::cout << buffer[cursor_pos] << std::flush;
        ++cursor_pos;
    };
    auto moveCmdCursorLeft = [&]() {
        --cursor_pos;
        std::cout << "\b" << std::flush;
    };
    auto deleteChar = [&]() {
        buffer.erase(cursor_pos - 1, 1);
        --cursor_pos;
        std::cout << "\b\033[s" << buffer.substr(cursor_pos) << ' ' << "\033[u" << std::flush;
    };
    auto deleteCharBack = [&]() {
        buffer.erase(cursor_pos, 1);
        std::cout << "\033[s"; // save cursor pos
        std::cout << buffer.substr(cursor_pos) << ' ';
        std::cout << "\033[u" << std::flush; // restore cursor pos
    };

    int ch;
    while (true) {
        size_t buffer_size = buffer.size();
        ch = readKey();

        switch (ch) {
        case KEY_ESC:
            return "";
            break;
        case KEY_ARROW_LEFT:
            if (cursor_pos > 0) {
                moveCmdCursorLeft();
            } else {
                std::cout << "\a" << std::flush;
            }
            break;
        case KEY_ARROW_RIGHT:
            if (cursor_pos < buffer_size) {
                moveCmdCursorRight();
            } else {
                std::cout << "\a" << std::flush;
            }
            break;
        case KEY_ARROW_UP:
            if (historyPosition > 0) {
                historyPosition--;

                // Buffer & screen clear
                while (cursor_pos > 0) {
                    deleteChar();
                }
                buffer.clear();

                size_t history_read_pos = historyPosition;
                std::string history_commad = commandHistory.at(history_read_pos);
                for (const char ch : history_commad) {
                    writeBuffer(buffer, cursor_pos, ch);
                }
            }
            break;
        case KEY_ARROW_DOWN:
            if (historyPosition < commandHistory.size()) {
                historyPosition++;

                // Buffer & screen clear
                while (cursor_pos > 0) {
                    deleteChar();
                }
                buffer.clear();

                size_t history_read_pos = historyPosition - 1;
                std::string history_commad = commandHistory.at(history_read_pos);
                for (const char ch : history_commad) {
                    writeBuffer(buffer, cursor_pos, ch);
                }
            } else {
                while (cursor_pos > 0) {
                    deleteChar();
                }
                buffer.clear();
            }
            break;
        case KEY_CTRL_LEFT:
            if (cursor_pos > 0) {
                moveCmdCursorLeft();
            }
            while (cursor_pos > 0 && isspace(buffer[cursor_pos])) {
                moveCmdCursorLeft();
            }
            while (cursor_pos > 0 && !isspace(buffer[cursor_pos - 1])) {
                moveCmdCursorLeft();
            }
            break;
        case KEY_CTRL_RIGHT:
            while (cursor_pos < buffer_size && isspace(buffer[cursor_pos])) {
                moveCmdCursorRight();
            }
            while (cursor_pos < buffer_size && !isspace(buffer[cursor_pos])) {
                moveCmdCursorRight();
            }
            break;
        case KEY_HOME:
            while (cursor_pos > 0) {
                moveCmdCursorLeft();
            }
            break;
        case KEY_END:
            while (cursor_pos < buffer_size) {
                moveCmdCursorRight();
            }
            break;
        case KEY_DELETE:
            if (cursor_pos < buffer_size) {
                buffer.erase(cursor_pos, 1);
                std::cout << "\033[s"; // save cursor pos
                std::cout << buffer.substr(cursor_pos) << ' ';
                std::cout << "\033[u" << std::flush; // restore cursor pos
            }
            break;
        case KEY_BACKSPACE:
            if (cursor_pos > 0) {
                deleteChar();
            } else {
                std::cout << "\a" << std::flush;
            }
            break;
        case KEY_DELETE_WORD:
            if (cursor_pos > 0) {
                deleteChar();
            }
            while (cursor_pos > 0 && isspace(buffer[cursor_pos])) {
                deleteChar();
            }
            while (cursor_pos > 0 && !isspace(buffer[cursor_pos - 1])) {
                deleteChar();
            }
            break;
        case KEY_DELETE_WORD_BACK:
            while (cursor_pos < buffer_size && isspace(buffer[cursor_pos])) {
                deleteCharBack();
            }
            while (cursor_pos < buffer_size && !isspace(buffer[cursor_pos])) {
                deleteCharBack();
            }
            break;
        case KEY_DELETE_LINE:
            while (cursor_pos > 0) {
                deleteChar();
            }
            break;
        case KEY_ENTER:
            commandHistory.push_back(buffer);
            historyPosition = commandHistory.size();
            return buffer;
            break;
        default:
            if (std::isprint(ch)) {
                writeBuffer(buffer, cursor_pos, ch);
            }
        }
    }
    restoreTerminal();
    return buffer;
}
#endif

void TerminalManager::writeBuffer(std::string &buffer, size_t &cursor_pos, int ch) {
    buffer.insert(cursor_pos, 1, static_cast<char>(ch));
    std::cout << "\033[s" << buffer.substr(cursor_pos) << "\033[u" << static_cast<char>(ch) << std::flush;
    ++cursor_pos;
}