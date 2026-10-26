#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>

#include "cuefile.h"
#include "array.h"

bool consume(char** s, uint8_t bytes) {
    for (int i = 0; i < bytes; i++) {
        // Premature terminator
        if (**s == '\0') return false;

        (*s)++;
    }

    return **s != '\0';
}

void trimWL(char** s) {
    while (isspace((unsigned char)**s)) {
        (*s)++;
    }
}

static void trimWR(char *s) {
    size_t n = strlen(s);
    while (n && isspace((unsigned char)s[n - 1])) s[--n] = '\0';
}

static char* unquote(char* s) {
    if (*s != '"') return s;
    s++;
    char* end = strchr(s, '"');
    if (end) *end = '\0';
    return s;
}

static char* unquoteFile(char* s) {
    if (*s == '"') return unquote(s);
    char* e = s;
    while (*e && !isspace((unsigned char)*e)) e++;
    *e = '\0';
    return s;
}

uint8_t FCUE_ParseTimestamp(CueTimestamp* timestamp, char* stamp) {
    // Not an index
    if (strncmp("INDEX", stamp, 5) != 0)  {
        return 100;
    }

    if (!consume(&stamp, 6)) return 100;

    char ii[3] = {0}, mm[3] = {0}, ss[3] = {0}, ff[3] = {0};
    strncpy(ii, stamp, 2); if (!consume(&stamp, 3)) return 100;
    strncpy(mm, stamp, 2); if (!consume(&stamp, 3)) return 100;
    strncpy(ss, stamp, 2); if (!consume(&stamp, 3)) return 100;
    strncpy(ff, stamp, 2);

    timestamp->mm = atoi(mm);
    timestamp->ss = atoi(ss);
    timestamp->ff = atoi(ff);

    return atoi(ii);
}

bool FCUE_ParseCUE(char* path, CueFile* cue) {
    FILE *fp = fopen(path, "r");
    if (!fp) return false;

    char line[512];
    char currentFile[512];
    char sIndex[3] = {0};

    currentFile[0] = '\0';

    CueTrack currentTrack = {0};
    CueTrack* tracks = NULL;

    char *p;
    while (fgets(line, sizeof line, fp)) {
        p = line;
        trimWL(&p);
        // Blank line
        if (!*p) continue;

        if (strncmp("REM GENRE", p, 9) == 0) {
           consume(&p, 9);
           trimWL(&p);
           trimWR(p);
           p = unquote(p);
           cue->genre = malloc(strlen(p) + 1);
           strcpy(cue->genre, p);
        } else if (strncmp("REM DATE", p, 8) == 0) {
           consume(&p, 8);
           trimWL(&p);
           trimWR(p);

           cue->date = atoi(p);
        } else if (strncmp("REM DISCID", p, 10) == 0) {
           consume(&p, 10);
           trimWL(&p);
           trimWR(p);
           cue->discid = malloc(strlen(p) + 1);
           strcpy(cue->discid, p);
        } else if (strncmp("REM COMMENT", p, 11) == 0) {
           consume(&p, 11);
           trimWL(&p);
           trimWR(p);
           cue->comment = malloc(strlen(p) + 1);
           strcpy(cue->comment, p);
        } else if (strncmp("PERFORMER", p, 9) == 0 && currentTrack.trackNumber == 0) {
           consume(&p, 9);
           trimWL(&p);
           trimWR(p);
           p = unquote(p);
           cue->performer = malloc(strlen(p) + 1);
           strcpy(cue->performer, p);
        } else if (strncmp("TITLE", p, 5) == 0 && currentTrack.trackNumber == 0) {
           consume(&p, 5);
           trimWL(&p);
           trimWR(p);
           p = unquote(p);
           cue->title = malloc(strlen(p) + 1);
           strcpy(cue->title, p);
        } else if (strncmp("FILE", p, 4) == 0) {
           memset(currentFile, 0, sizeof currentFile);

           consume(&p, 4);
           trimWL(&p);
           trimWR(p);
           strcpy(currentFile, unquoteFile(p));
        } else if (strncmp("TRACK", p, 5) == 0) {
            if (currentTrack.trackNumber != 0) {
                arr_push(tracks, currentTrack);
                currentTrack = (CueTrack) { 0 };
            }
            currentTrack.file = malloc(strlen(currentFile) + 1);
            strcpy(currentTrack.file, currentFile);

           consume(&p, 5);
           trimWL(&p);

           strncpy(sIndex, p, 2);
           sIndex[2] = '\0';
           currentTrack.trackNumber = atoi(sIndex);
        } else if (strncmp("TITLE", p, 5) == 0) {
           consume(&p, 5);
           trimWL(&p);
           trimWR(p);
           p = unquote(p);

           currentTrack.title = malloc(strlen(p) + 1);
           strcpy(currentTrack.title, p);
        } else if (strncmp("PERFORMER", p, 9) == 0) {
           consume(&p, 9);
           trimWL(&p);
           trimWR(p);
           p = unquote(p);

           currentTrack.performer = malloc(strlen(p) + 1);
           strcpy(currentTrack.performer, p);
        } else if (strncmp("ISRC", p, 4) == 0) {
           consume(&p, 4);
           trimWL(&p);
           trimWR(p);

           currentTrack.isrc = malloc(strlen(p) + 1);
           strcpy(currentTrack.isrc, p);
        } else if (strncmp("INDEX", p, 5) == 0) {
           trimWR(p);

           CueTimestamp timestamp;
           uint8_t index = FCUE_ParseTimestamp(&timestamp, p);

           if (index == 0) {
                currentTrack.index00 = timestamp;
           } else if (index == 1) {
                currentTrack.index01 = timestamp;
           } else {
               printf("Unsupported subindex: %u\n", index);
           }
        }
    }

    // Don't lose the last track
    if (currentTrack.trackNumber != 0) {
        arr_push(tracks, currentTrack);
    }

    fclose(fp);

    cue->tracks = tracks;
    return true;
}

static const char* str(const char* s) {
    return s ? s : "";
}
