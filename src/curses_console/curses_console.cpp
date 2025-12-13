//
// Created by sean on 12/20/24.

#include "curses_console.hpp"

#include <ncurses.h>

#include <stdexcept>
#include <string>

CursesConsole::CursesConsole() {
    // This should equal stdscr.
    mScr = initscr();

    if (mScr == nullptr) {
        throw std::runtime_error("Failed to initialize curses window.");
    }

    // Disable echoing.
    noecho();
    // Allow capturing special keys.
    keypad(mScr, TRUE);

    // Initialize colors.
    start_color();
    // Create explicit color pairs.
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_BLUE, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    init_pair(5, COLOR_GREEN, COLOR_BLACK);
}

CursesConsole::~CursesConsole() {
    // Restore terminal state.
    endwin();
}

void CursesConsole::noInputBuffer() {
    // Disable input buffering.
    cbreak();
}

void CursesConsole::nonBlockingGetCh() {
    nodelay(stdscr, true);
}

void CursesConsole::blockingGetCh(int timeoutMs) {
    // Make getCh calls block again.
    wtimeout(stdscr, timeoutMs);
    // We store this in case we temporarily change,
    // it, e.g., when reading a full string.
    mLastBlockingTime = timeoutMs;
}

void CursesConsole::whiteOnBlack() {
    wattron(mScr, COLOR_PAIR(1));
}

void CursesConsole::blueOnBlack() {
    wattron(mScr, COLOR_PAIR(2));
}

void CursesConsole::redOnBlack() {
    wattron(mScr, COLOR_PAIR(3));
}

void CursesConsole::yellowOnBlack() {
    wattron(mScr, COLOR_PAIR(4));
}

void CursesConsole::greenOnBlack() {
    wattron(mScr, COLOR_PAIR(5));
}

void CursesConsole::writeBuffer() {
    wrefresh(mScr);
}

void CursesConsole::clearBuffer() {
    wclear(mScr);
}

void CursesConsole::moveCursor(int x, int y) {
    wmove(mScr, y, x);
}

void CursesConsole::addChar(const char ch) {
    waddch(mScr, ch);
}

void CursesConsole::addString(const std::string &str) {
    for (const auto &ch : str) {
        waddch(mScr, ch);
    }
}

void CursesConsole::addStringWithColor(const std::string &str, ColorPair color) {
    wattron(mScr, COLOR_PAIR(static_cast<char>(color)));
    addString(str);
    whiteOnBlack();
}

[[nodiscard]] std::pair<int, int> CursesConsole::getScreenSize() const {
    int rows, cols;
    getmaxyx(mScr, rows, cols);
    return {rows, cols};
}

// NOTE: These are not static because they assume
// the initialization that is done in the constructor.

int CursesConsole::getChar() {
    return getch();
}

std::string CursesConsole::getString() {
    // Make input behave like normal console.
    echo();
    int savedBlockingTime = mLastBlockingTime;
    blockingGetCh();

    char buffer[GET_STRING_BUFFER_SIZE]{};
    wgetnstr(mScr, buffer, GET_STRING_BUFFER_SIZE - 1);

    // Restore our input settings.
    noecho();
    blockingGetCh(savedBlockingTime);

    // Discard any inputs still in buffer.
    flushinp();

    return std::string{buffer};
}

void CursesConsole::cursorVisible(bool visible) {
    curs_set(visible ? 1 : 0);
}
