//
// Created by sean on 12/20/24.
//

#include "curses_console.h"

#include <ncurses.h>

#include <stdexcept>

CursesConsole::CursesConsole() {
  // This should be stdscr.
  scr = initscr();

  if (scr == nullptr) {
    throw std::runtime_error( "Failed to initialize curses window." );
  }

  // Disable echoing.
  noecho();
  // Allow capturing special keys.
  keypad( scr, TRUE );

  // Initialize colors.
  start_color();
  // Create explicit color pairs.
  init_pair( 1, COLOR_WHITE, COLOR_BLACK );
  init_pair( 2, COLOR_BLUE, COLOR_BLACK );
  init_pair( 3, COLOR_RED, COLOR_BLACK );
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
  nodelay( stdscr, true );
}

void CursesConsole::blockingGetCh(int timeoutMs) {
  // Make getCh calls block again.
  wtimeout( stdscr, timeoutMs );
}

void CursesConsole::whiteOnBlack() {
  wattron( scr, COLOR_PAIR( 1 ) );
}

void CursesConsole::blueOnBlack() {
  wattron( scr, COLOR_PAIR( 2 ) );
}

void CursesConsole::redOnBlack() {
  wattron( scr, COLOR_PAIR( 3 ) );
}

void CursesConsole::writeBuffer() {
  wrefresh( scr );
}

void CursesConsole::clearBuffer() {
    wclear( scr );
}

void CursesConsole::moveCursor( int x, int y ) {
  wmove( scr, y, x );
}

void CursesConsole::addChar( const char ch ) {
  waddch( scr, ch );
}

void CursesConsole::addString( const std::string &str ) {
  for (const auto &ch : str) {
    waddch( scr, ch );
  }
}

void CursesConsole::addStringWithColor( const std::string &str, ColorPair color ) {
  switch (color) {
    case ColorPair::BlueOnBlack:
      blueOnBlack();
      break;
    case ColorPair::RedOnBlack:
      redOnBlack();
      break;
    default:
      throw std::runtime_error( "Unknown color pair." );
  }
  addString( str );
  whiteOnBlack();
}

// NOTE: These are not static because they assume
// the initialization that is done in the constructor.

int CursesConsole::getChar() {
  return getch();
}

void CursesConsole::cursorVisible( bool visible ) {
  curs_set( visible ? 1 : 0 );
}
