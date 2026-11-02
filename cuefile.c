#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "cuefile.h"

static const StringView REM_GENRE   = SV("REM GENRE");
static const StringView REM_DATE    = SV("REM DATE");
static const StringView REM_DISCID  = SV("REM DISCID");
static const StringView REM_COMMENT = SV("REM COMMENT");
static const StringView PERFORMER   = SV("PERFORMER");
static const StringView TITLE       = SV("TITLE");
static const StringView FILE_KW     = SV("FILE");
static const StringView TRACK       = SV("TRACK");
static const StringView ISRC        = SV("ISRC");
static const StringView INDEX       = SV("INDEX");

static StringView value(StringView s) {
  str_trim_by_predicate(&s, isspace);
  if (str_eat_char(&s, '"'))
    return str_split_by_delim(&s, '"');
  return s;
}

static StringView token(StringView *s) {
  str_trim_by_predicate(s, isspace);
  if (str_eat_char(s, '"'))
    return str_split_by_delim(s, '"');
  return str_split_by_predicate(s, isspace);
}

// Parses "INDEX ii mm:ss:ff". Returns the index number, or 100 on failure.
uint8_t FCUE_ParseTimestamp(CueTimestamp *timestamp, StringView line) {
  int ii, mm, ss, ff;

  if (!str_eat_prefix(&line, INDEX))
    return 100;
  str_trim_by_predicate(&line, isspace);

  if (!str_parse_uint(&line, &ii))
    return 100;
  str_trim_by_predicate(&line, isspace);

  if (!str_parse_uint(&line, &mm) || !str_eat_char(&line, ':') ||
      !str_parse_uint(&line, &ss) || !str_eat_char(&line, ':') ||
      !str_parse_uint(&line, &ff))
    return 100;

  timestamp->mm = mm;
  timestamp->ss = ss;
  timestamp->ff = ff;
  return (uint8_t)ii;
}

void FCUE_FreeCUE(CueFile *cue) {
  for (int i = 0; i < arr_len(cue->tracks); i++) {
    free(cue->tracks[i].title);
    free(cue->tracks[i].performer);
    free(cue->tracks[i].file);
    free(cue->tracks[i].isrc);
  }

  arr_free(cue->tracks);
  free(cue);
}

bool FCUE_ParseCUE(char *path, CueFile *cue) {
  FILE *fp = fopen(path, "r");
  if (!fp)
    return false;

  char line[1024];
  char *currentFile = NULL;
  bool inTrack = false;

  CueTrack currentTrack = {0};
  CueTrack *tracks = NULL;

  while (fgets(line, sizeof line, fp)) {
    StringView l = str_of_cstr(line);
    str_trim_by_predicate(&l, isspace);
    if (l.count == 0)
      continue;

    if (str_eat_prefix(&l, REM_GENRE)) {
      cue->genre = str_to_cstr(value(l));
    } else if (str_eat_prefix(&l, REM_DATE)) {
      int year;
      str_trim_by_predicate(&l, isspace);
      if (str_parse_uint(&l, &year))
        cue->date = year;
    } else if (str_eat_prefix(&l, REM_DISCID)) {
      cue->discid = str_to_cstr(value(l));
    } else if (str_eat_prefix(&l, REM_COMMENT)) {
      cue->comment = str_to_cstr(value(l));
    } else if (str_eat_prefix(&l, PERFORMER)) {
      char *v = str_to_cstr(value(l));
      if (inTrack) currentTrack.performer = v;
      else         cue->performer = v;
    } else if (str_eat_prefix(&l, TITLE)) {
      char *v = str_to_cstr(value(l));
      if (inTrack) currentTrack.title = v;
      else         cue->title = v;
    } else if (str_eat_prefix(&l, FILE_KW)) {
      free(currentFile);
      currentFile = str_to_cstr(token(&l));
    } else if (str_eat_prefix(&l, TRACK)) {
      if (inTrack)
        arr_push(tracks, currentTrack);
      currentTrack = (CueTrack){0};
      inTrack = true;

      if (currentFile)
        currentTrack.file = str_to_cstr(str_of_cstr(currentFile));

      int n;
      str_trim_by_predicate(&l, isspace);
      if (str_parse_uint(&l, &n))
        currentTrack.trackNumber = n;
    } else if (str_eat_prefix(&l, ISRC)) {
      currentTrack.isrc = str_to_cstr(value(l));
    } else if (str_starts_with(l, INDEX)) {
      CueTimestamp ts;
      uint8_t index = FCUE_ParseTimestamp(&ts, l);

      if (index == 0)       currentTrack.index00 = ts;
      else if (index == 1)  currentTrack.index01 = ts;
      else                  printf("Unsupported subindex: %u\n", index);
    }
  }

  if (inTrack)
    arr_push(tracks, currentTrack);

  free(currentFile);
  fclose(fp);

  cue->tracks = tracks;
  return true;
}
