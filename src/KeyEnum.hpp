#pragma once
#ifdef __unix__
enum Key {
    KEY_NULL = 0,
    KEY_ESC = 27,
    KEY_ENTER = '\r',
    KEY_BACKSPACE = 127,

    KEY_ARROW_LEFT = 1000,
    KEY_ARROW_RIGHT,
    KEY_ARROW_UP,
    KEY_ARROW_DOWN,
    KEY_HOME,
    KEY_END,
    KEY_CTRL_LEFT,
    KEY_CTRL_RIGHT,

    KEY_DELETE,
    KEY_DELETE_WORD,
    KEY_DELETE_WORD_BACK,
    KEY_DELETE_LINE
    // Add more as needed
};
#endif // __unix__