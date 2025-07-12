// Provides a wrapper around ncurses functionality.
//
// Created by sean on 12/20/24.
//

#ifndef CURSES_CONSOLE_H
#define CURSES_CONSOLE_H

#include <cstdint>
#include <string>
#include <utility>

// We include these to allow moving full ncurses
// header include to the implementation file, so
// clients don't get all of its declarations.

typedef struct _win_st WINDOW;

#define KEY_DOWN 0402  // down-arrow key
#define KEY_UP 0403    // up-arrow key
#define KEY_LEFT 0404  // left-arrow key
#define KEY_RIGHT 0405 // right-arrow key

#define CURSES_KEY_d 0x64 // This is the ASCII code.
#define CURSES_KEY_f 0x66
#define CURSES_KEY_l 0x6C
#define CURSES_KEY_p 0x70
#define CURSES_KEY_q 0x71
#define CURSES_KEY_s 0x73

// ---------------------------------------------
// A class providing a C++ interface to ncurses.

class CursesConsole {
  public:
    static constexpr int NO_KEY = -1;
    // getCh treats negative blocking times as infinite.
    static constexpr int INFINITE_BLOCKING = -1;

    enum class ColorPair : std::uint8_t
    {
        WhiteOnBlack = 1,
        BlueOnBlack,
        RedOnBlack,
        YellowOnBlack,
        GreenOnBlack,
    };

  public:
    // Initializes curses console state.
    CursesConsole();

    // Calls curses func to restore console.
    ~CursesConsole();

    // Disables input buffering, "making characters typed
    // by the user immediately available to the program".
    // See: https://linux.die.net/man/3/cbreak
    void noInputBuffer();

    // Default argument sets blocking mode.
    void blockingGetCh(int timeoutMs = INFINITE_BLOCKING);

    void nonBlockingGetCh();

    // Set current text/background colors.
    void whiteOnBlack();

    void blueOnBlack();

    void redOnBlack();

    void yellowOnBlack();

    void greenOnBlack();

    void writeBuffer();

    void clearBuffer();

    void moveCursor(int x, int y);

    void addChar(char ch);

    void addString(const std::string &str);

    void addStringWithColor(const std::string &str, ColorPair color);

    [[nodiscard]] std::pair<int, int> getScreenSize() const;

    // NOTE: These methods are not static because they assume the
    // initialization that is done in the constructor, so they need
    // to be called after a class instance has been constructed.

    int getChar();

    std::string getString();

    void cursorVisible(bool visible);

  private:
    WINDOW *mScr;
    int mLastBlockingTime = INFINITE_BLOCKING;

    static constexpr std::size_t GET_STRING_BUFFER_SIZE = 1024;
};

#endif // CURSES_CONSOLE_H
