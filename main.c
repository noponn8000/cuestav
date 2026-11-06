#include <string.h>
#include <unistd.h>
#define _XOPEN_SOURCE_EXTENDED 1

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <ncurses.h>
#include <locale.h>
#include <panel.h>

#include "file_tree.h"
#include "miniaudio.h"
#include "array.h"
#include "cuefile.h"

// Note: tilde is not expanded in this path. That is done by the shell.
#define DEFAULT_ROOT_DIRECTORY "/home/ferenc/Music"

#define TITLE_HEIGHT 2
#define TITLE_STRING ".CUEstav Music Player"
#define TITLE_STRING_LENGTH 22
#define PLAYBACK_HEIGHT 2
#define MARGIN_L 1
#define MARGIN_R 1

const wchar_t* BAR_EMPTY = L"\xe29691";
const wchar_t* BAR_FILLED = L"\xe29688";

char CURRENT_DIR[PATH_MAX + 1];

/* Miniaudio */
ma_result result;
ma_engine audioEngine;
ma_decoder decoder;
ma_sound sound;

CueTrack currentTrack = {0};
CueFile* currentFile = NULL;
int nextTrackIndex = -1;

bool playing = false;

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

FileView* change_dir(FileView* files, int i) {
  if (i >= 0 && i < arr_len(files) && !files[i].isRegular) {
    memset(CURRENT_DIR, 0, sizeof(CURRENT_DIR));
    strcpy(CURRENT_DIR, files[i].path);
    chdir(CURRENT_DIR);
    return get_files(files[i].path);
  }

  return NULL;
}

CueFile* openCue(FileView* files, int i) {
  if (i >= 0 && i < arr_len(files) && files[i].isRegular && get_file_type(files[i].path) == CUE) {
      CueFile *cue = malloc(sizeof(CueFile));
      if (FCUE_ParseCUE(files[i].path, cue)) {
        return cue;
      } else {
        free(cue);
        return NULL;
      }
  }

  return NULL;
}

ma_uint64 CueToPCM(CueTimestamp t, ma_uint32 sampleRate) {
    ma_uint64 cdFrames = ((ma_uint64)t.mm * 60 + t.ss) * 75 + t.ff;
    return cdFrames * sampleRate / 75;
}

float PCMToSeconds(ma_uint64 pcm, ma_uint32 sampleRate) {
	return ((float) pcm / sampleRate);
}

void getTrackDuration(CueFile* file, int index, ma_uint64* start, ma_uint64* end) {
    CueTrack current = file->tracks[index];

    ma_decoder probe;
    if (ma_decoder_init_file(current.file, NULL, &probe) != MA_SUCCESS) {
        printf("failed to open %s\n", current.file);
        return;
    }
    ma_uint32 fileRate = probe.outputSampleRate;
    ma_uint64 fileLen = 0;
    ma_decoder_get_length_in_pcm_frames(&probe, &fileLen);
    ma_decoder_uninit(&probe);

    *start = CueToPCM(current.index01, audioEngine.sampleRate);

    bool hasNext = index + 1 < arr_len(file->tracks) &&
                   strcmp(file->tracks[index + 1].file, current.file) == 0;
    if (hasNext) {
        *end = CueToPCM(file->tracks[index + 1].index01, audioEngine.sampleRate);
        nextTrackIndex = index + 1;
    }
    else {
        *end = fileLen * (float) audioEngine.sampleRate / fileRate;
        nextTrackIndex = -1;
    }
}

void playTrack(CueFile* file, int index) {
	if (playing) {
    	ma_sound_uninit(&sound); 
	}
    
    result = ma_sound_init_from_file(&audioEngine, file->tracks[index].file, MA_SOUND_FLAG_STREAM,
                                     NULL, NULL, &sound);
    if (result != MA_SUCCESS) {
        printf("failed to init sound, code %d\n", result);
        return;
    }

	ma_uint64 start; ma_uint64 end;
	getTrackDuration(file, index, &start, &end);
    ma_sound_seek_to_pcm_frame(&sound, start);

	currentFile = file;
    currentTrack = file->tracks[index]; 
    currentTrack.end = end;
    currentTrack.start = start;

    ma_sound_start(&sound);
    playing = true;
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

  *left = newwin(*wly, *wlx, TITLE_HEIGHT, MARGIN_L);
  *right = newwin(*wry, *wrx, TITLE_HEIGHT, splitx);
}

bool getCueTrack(CueFile** cues, int listIndex, int* fileIndex, int* trackIndex) {
    int acc = 0;
    for (int i = 0; i < arr_len(cues); i++) {
		acc++;
		*fileIndex = i;
		for (int j = 0; j < arr_len(cues[i]->tracks); j++) {
    		if (acc == listIndex) {
				*trackIndex = j;
				return true;
    		}
    		acc++;
		}
    }
    *trackIndex = -1; *fileIndex = -1;
    return false;
}

void printUnicode(const char *str) {
    size_t len = mbstowcs(NULL, str, 0);

    if (len == (size_t) -1) {
        return;
    }

    wchar_t *widestr = malloc((len + 1) * sizeof(wchar_t));
    if (widestr == NULL)
        return;

    mbstowcs(widestr, str, len + 1);
    addwstr(widestr);

    free(widestr);
}

int main(int argc, char** argv) {
  char* path = DEFAULT_ROOT_DIRECTORY;
  DIR* dir;
  
  /* Parse args */
  if (argc > 1) {
    path = argv[1];
  }
  strcpy(CURRENT_DIR, path);

  /* Miniaudio init */
  if (ma_engine_init(NULL, &audioEngine) != MA_SUCCESS) { fprintf(stderr, "engine init failed\n"); return 1; }

  FileView* files = get_files(path);
  CueFile** cues = NULL;
  // We have to init manually or otherwise arr_len will SIGSEV
  arr_init(cues);

  WINDOW *win_left;
  WINDOW *win_right;
  // Window dimensions
  int wlx, wly, wrx, wry;

  bool tabLeft = true;
  bool paused = true;
  int listIndexLeft = 0;
  int listIndexRight = 0;
  int listIndex = 0;

  // User input
  int ch;

  setlocale(LC_ALL, "");
  initscr(); // Start curses
  cbreak();
  keypad(stdscr, TRUE);  // Get special keys
  noecho(); // Don't echo input
  timeout(100); // Timeout getch() after 100ms

  if (has_colors() == FALSE) {
    printf("Your terminal has no color support! Exiting.\n");
    exit(1);
  }

  start_color();
  init_pair(1, COLOR_YELLOW, COLOR_BLACK);
  init_pair(2, COLOR_YELLOW, COLOR_BLACK);
  init_pair(3, COLOR_GREEN, COLOR_BLACK);

  make_windows(&win_left, &win_right, &wlx, &wly, &wrx, &wry); 

  ch = '.';
  do {
    switch (ch) {
      case KEY_RESIZE:
        make_windows(&win_left, &win_right, &wlx, &wly, &wrx, &wry); 
        clearok(stdscr, TRUE);
        break;
      case 'j':
        listIndex++;
        break;
      case 'k':
        listIndex--;
        break;
      case 'c':
        for (int i = 0; i < arr_len(cues); i++) {
          FCUE_FreeCUE(cues[i]);
        }
        arr_free(cues);
        arr_init(cues);
        listIndexRight = 0;
        if (!tabLeft) listIndex = 0;
        break;
      case '\t':
        tabLeft = !tabLeft;
        if (tabLeft) {
          listIndexRight = listIndex;
          listIndex = listIndexLeft;
        } else {
          listIndexLeft = listIndex;
          listIndex = listIndexRight;
        }
        break;
       case ' ':
       	if (playing) {
			ma_sound_stop(&sound);
       	}
       	else {
           if (currentTrack.trackNumber > 0) {
               ma_sound_start(&sound);
           }
       	}
       	playing = !playing;
       	break;
      // return key. doesn't work with KEY_ENTER for some reason
      case 10:
        if (tabLeft) {
          if (listIndex < arr_len(files) && listIndex >= 0 && !files[listIndex].isRegular) {
          FileView *newFiles = change_dir(files, listIndex);
            if (newFiles != NULL) {
              arr_free(files);
              files = newFiles;
            }
          } else {
            CueFile* cue = openCue(files, listIndex);
            if (cue != NULL) {
              arr_push(cues, cue);
            }
          }
        } else {
          int trackIndex; int fileIndex;
          if (getCueTrack(cues, listIndex, &fileIndex, &trackIndex)) {
              playTrack(cues[fileIndex], trackIndex);
          }
        }
        break;
    }

    if (playing && ma_sound_at_end(&sound)) {
		if (nextTrackIndex != -1) {
    		playTrack(currentFile, nextTrackIndex);
		} else {
    		playing = false;
    		currentFile = NULL;
    		currentTrack = (CueTrack) {0};
		}
    }

    if (tabLeft) {
      if (listIndex < 0) {
        listIndex = arr_len(files) > 0 ? arr_len(files) - 1 : 0;
      } else if (listIndex  >= arr_len(files)) {
        listIndex = 0;
      }
    } else {
      int sum = 0;
      for (int i = 0; i < arr_len(cues); i++) {
        sum += 1 + arr_len(cues[i]->tracks);
      }

      if (listIndex < 0) {
        listIndex = sum > 0 ? sum - 1 : 0;
      } else if (listIndex >= sum) {
        listIndex = 0;
      }
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
    curs_set(0);

    // Draw files
    for (int i = 0; i < arr_len(files) && i < wly - 3; i++) {
      wmove(win_left, i + 2, 1);

      if (files[i].isRegular) {
        wattron(win_left, COLOR_PAIR(2));
      } else {
        wattron(win_left, COLOR_PAIR(3));
      }

      if (tabLeft && listIndex == i) {
        wattron(win_left, A_BOLD);
      }

      waddnstr(win_left, files[i].path, wlx - 4);

      wattroff(win_left, COLOR_PAIR(2) | COLOR_PAIR(3) | A_BOLD);
    }

    // Draw cuesheets

    CueFile *cue;
    CueTrack track;
    int lines = 0;
    for (int i = 0; i < arr_len(cues); i++) {
      cue = cues[i];
      wmove(win_right, lines + 2, 1);

      if (!tabLeft && listIndex == lines) {
        wattron(win_right, A_BOLD);
      }

      wattron(win_right, A_ITALIC | COLOR_PAIR(3));
      waddnstr(win_right, cue->title, wrx - 4);
      wattroff(win_right, A_ITALIC | COLOR_PAIR(3) | A_BOLD);
      lines++;

      for (int j = 0; j < arr_len(cue->tracks); j++) {
        track = cue->tracks[j];

        wattron(win_right, COLOR_PAIR(2));
        if (!tabLeft && listIndex == lines) {
          wattron(win_right, A_BOLD);
        }

        wmove(win_right, lines + 2, 5);
        waddnstr(win_right, track.title, wrx - 6);
        wattroff(win_right, COLOR_PAIR(2) | A_BOLD);
        lines++;
      }
    }

    wattron(win_right, A_BOLD);
    mvwprintw(win_right, 1, COLS / 4 - 4, ".CUEsheet");
    wattroff(win_right, A_BOLD);

    attron(COLOR_PAIR(1) | A_BLINK);
    mvprintw(0, COLS / 2 - TITLE_STRING_LENGTH / 2, "%s", TITLE_STRING);
    attroff(COLOR_PAIR(1) | A_BLINK);

    mvprintw(1, 1, "Current directory: %s", CURRENT_DIR);

	if (playing) {
        mvprintw(LINES - 2, 2, "▶");
	} else {
        mvprintw(LINES - 2, 2, "⏸");
	}

	if (currentTrack.trackNumber > 0) {
    	float pTrack = PCMToSeconds((uint64_t) currentTrack.end - currentTrack.start, audioEngine.sampleRate);
    	float pCursor = 0;
    	ma_sound_get_cursor_in_seconds(&sound, &pCursor);
    	pCursor -= PCMToSeconds(currentTrack.start, audioEngine.sampleRate);

		move(LINES - 1, 4);
    	printw("%02u:%02u/%02u:%02u", (int) pCursor / 60, (int) pCursor % 60, (int) pTrack / 60, (int) pTrack % 60);

    	int barWidth = COLS - 12;
    	for (int i = 0; i < (pCursor/pTrack) * barWidth; i++) {
        	move(LINES - 1, 16 + i);
        	add_wch(WACS_BLOCK);
    	}
	}

	if (currentTrack.trackNumber > 0) {
    	move(LINES - 2, 4);
    	addnstr(currentTrack.title, COLS - 6);
	}

    // Queue refreshes in the order parent -> child
    wnoutrefresh(stdscr);
    wnoutrefresh(win_left);
    wnoutrefresh(win_right);

    doupdate();
  } while ((ch = getch()) != 'q');

  arr_free(files);

  delwin(win_left);
  delwin(win_right);
  endwin();

  return EXIT_SUCCESS;
}
