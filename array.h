#include <stdlib.h>

#define ARR_INIT_CAPACITY 16

typedef struct {
  size_t count;
  size_t capacity;
} Header;

#define arr_push(arr, x)                                                           \
  do {                                                                             \
    if (arr == NULL) {                                                             \
      Header* header = malloc(sizeof(*arr) * ARR_INIT_CAPACITY + sizeof(Header));  \
      header->count = 0;                                                           \
      header->capacity = ARR_INIT_CAPACITY;                                        \
      arr = (void*)(header + 1);                                                   \
    }                                                                              \
    Header* header = (Header*)(arr) - 1;                                           \
    if (header->count >= header->capacity) {                                       \
      header->capacity *= 2;                                                       \
      header = realloc(header, sizeof(*arr) * header->capacity + sizeof(Header)); \
      arr = (void*)(header + 1);                                                   \
    }                                                                              \
    (arr)[header->count++] = (x);                                                  \
  } while(0) 

#define arr_len(arr)                                                               \
 ((Header*)(arr) - 1)->count

#define arr_free(arr)                                                              \
  free((Header*)(arr) - 1)
