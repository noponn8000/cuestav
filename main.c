#define _XOPEN_SOURCE_EXTENDED 1

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <ncurses.h>
#include <locale.h>
#include <panel.h>

#include "file_tree.h"
#include "array.h"

// Note: tilde is not expanded in this path. That is done by the shell.
#define DEFAULT_ROOT_DIRECTORY "/home/ferenc/Music"

#define TITLE_HEIGHT 2
#define TITLE_STRING ".CUEstav Music Player"
#define TITLE_STRING_LENGTH 22
#define PLAYBACK_HEIGHT 2

void set_win_border(WINDOW *win) {
    cchar_t ls, rs, ts, bs;
    cchar_t tl, tr, bl, br;

    setcchar(&ls, L"│", 0, 0, NULL);
    setcchar(&rs, L"│", 0, 0, NULL);
    setcchar(&ts, L"─", 0, 0, NULL);
    setcchar(&bs, L"─", 0, 0, NULL);

    setcchar(&tl, L"┌", 0, 0, NULL);
    setcchar(&tr, L"┐", 0, 0, NULL);
    setcchar(&bl, L"└", 0, 0, NULL);
    setcchar(&br, L"┘", 0, 0, NULL);

    wborder_set(win, &ls, &rs, &ts, &bs,
                &tl, &tr, &bl, &br);
}

void make_windows(WINDOW** left, WINDOW** right, int* wlx, int* wly, int* wrx, int* wry) {
  if (left != NULL)
    delwin(*left);
  if (right != NULL)
    delwin(*right);

  int splitx = COLS / 2;
  *wlx = splitx - 1;
  *wly = LINES - TITLE_HEIGHT - PLAYBACK_HEIGHT;
  *wrx = splitx;
  *wry = LINES - TITLE_HEIGHT - PLAYBACK_HEIGHT;

  *left = newwin(*wly, *wlx, TITLE_HEIGHT, 0);
  *right = newwin(*wry, *wrx, TITLE_HEIGHT, splitx);
}

int main(int argc, char** argv) {
  char* path = DEFAULT_ROOT_DIRECTORY;
  DIR* dir;
  
  /* Parse args */
  if (argc > 1) {
    path = argv[1];
  }

  FileView* files = get_files(path);

  WINDOW *win_left;
  WINDOW *win_right;
  int wlx, wly, wrx, wry;

  int ch;

  setlocale(LC_ALL, "");
  initscr(); // Start curses
  cbreak();
  keypad(stdscr, TRUE);  // Get special keys
  noecho(); // Don't echo input

  if (has_colors() == FALSE) {
    printf("Your terminal has no color support! Exiting.\n");
    exit(1);
  }

  start_color();
  init_pair(1, COLOR_YELLOW, COLOR_BLACK);
  init_pair(2, COLOR_RED, COLOR_BLACK);
  init_pair(3, COLOR_BLUE, COLOR_BLACK);

  make_windows(&win_left, &win_right, &wlx, &wly, &wrx, &wry); 

  ch = ' ';
  do {
    if (ch == KEY_RESIZE) {
      make_windows(&win_left, &win_right, &wlx, &wly, &wrx, &wry); 
      clearok(stdscr, TRUE);
    }

    werase(stdscr);
    werase(win_left);
    werase(win_right);

    set_win_border(win_left);   
    set_win_border(win_right);

    // Draw title of left window
    wattron(win_left, A_BOLD);
    mvwprintw(win_left, 1, COLS / 4 - 2, "Files");
    wattroff(win_left, A_BOLD);

    // Draw files
    for (int i = 0; i < arr_len(files) && i < wly - 3; i++) {
      wmove(win_left, i + 2, 1);

      if (files[i].isRegular) {
        wattron(win_left, COLOR_PAIR(2));
      } else {
        wattron(win_left, COLOR_PAIR(3));
      }
      waddnstr(win_left, files[i].path, wlx - 4);

      wattroff(win_left, COLOR_PAIR(2) | COLOR_PAIR(3) | A_BOLD);
    }

    wattron(win_right, A_BOLD);
    mvwprintw(win_right, 1, COLS / 4 - 4, ".CUEsheet");
    wattroff(win_right, A_BOLD);

    attron(COLOR_PAIR(1) | A_BLINK);
    mvprintw(0, COLS / 2 - TITLE_STRING_LENGTH / 2, "%s", TITLE_STRING);
    attroff(COLOR_PAIR(1) | A_BLINK);

    // Queue refreshes in the order parent -> child
    wnoutrefresh(stdscr);
    wnoutrefresh(win_left);
    wnoutrefresh(win_right);

    doupdate();
  } while ((ch = getch()) != 'q');

  delwin(win_left);
  delwin(win_right);
  endwin();

  return EXIT_SUCCESS;
}
