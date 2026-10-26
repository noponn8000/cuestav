#include <stdint.h>

typedef struct {
    uint8_t mm;
    uint8_t ss;
    uint8_t ff;
} CueTimestamp;

typedef struct {
    uint8_t trackNumber;
    char* file;
    CueTimestamp index00;
    CueTimestamp index01;
    char* title;
    char* performer;
    char* isrc;
    
} CueTrack;
