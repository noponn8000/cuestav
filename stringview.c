#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#include "stringview.h"

void str_trim_left(StringView *sv, size_t n) {
  if (n > sv->count) n = sv->count;

  sv->count -= n;
  sv->data += n;
}

void str_trim_right(StringView *sv, size_t n) {
  if (n > sv->count) n = sv->count;

  sv->count -= n;
}

StringView str_split_by_delim(StringView *sv, char delim) {
  size_t i = 0;
  while (i < sv->count && sv->data[i] != delim) {
      i += 1;
  }

  if (i < sv->count) {
    StringView result = {
      .data = sv->data,
      .count = i
    };
    str_trim_left(sv, i + 1);
    return result;
  }

  StringView result = *sv;
  str_trim_left(sv, sv->count);
  return result; 
}

void str_trim_by_predicate(StringView *sv, int (*istype)(int c)) {
  while (sv->count > 0 && istype(sv->data[0])) {
    sv->data++; sv->count--;
  }

  while (sv->count > 0 && istype(sv->data[sv->count - 1])) {
    sv->count--;
  }
}

void str_trim_by_delim(StringView *sv, char c) {
  while (sv->count > 0 && sv->data[0] == c) {
    sv->data++; sv->count--;
  }

  while (sv->count > 0 && c == sv->data[sv->count - 1]) {
    sv->count--;
  }
}

StringView str_split_by_predicate(StringView *sv, int (*istype)(int c)) {
  size_t i = 0;
  while (i < sv->count && !istype(sv->data[i])) {
      i += 1;
  }

  if (i < sv->count) {
    StringView result = {
      .data = sv->data,
      .count = i
    };
    str_trim_left(sv, i + 1);
    return result;
  }

  StringView result = *sv;
  str_trim_left(sv, sv->count);
  return result; 
}

StringView str_of_cstr(const char *cstr) {
  return (StringView) {
  .data = cstr,
  .count = strlen(cstr)
  };
}

bool str_starts_with(StringView a, StringView b) {
  if (a.count < b.count) return false;
  
  for (int i = 0; i < b.count; i++) {
    if (a.data[i] != b.data[i]) {
      return false;
    }
  }

  return true;
}

bool str_ends_with(StringView a, StringView b) {
  if (a.count < b.count) return false;

  for (int i = 0; i < b.count; i++) {
    if (a.data[a.count - i - 1] != b.data[b.count - i - 1]) {
      return false;
    }
  }  

  return true;
}

// If str starts with c, trim it away
bool str_eat_char(StringView* str, char c) {
  if (str->count >= 1 && str_char_at(*str, 0) == c) {
    str_trim_left(str, 1);
    return true;
  }

  return false;
}

// If str starts with prefix, trim it away
bool str_eat_prefix(StringView* str, StringView prefix) {
  if (str_starts_with(*str, prefix)) {
    str_trim_left(str, prefix.count);
    return true;
  }

  return false;
}

// If str starts with an unsigned int, parse it and trim it.
bool str_parse_uint(StringView* str, int* result) {
  size_t n = 0;
  int v = 0;

  while (n < str->count && isdigit((unsigned char) str_char_at(*str, n))) {
    v = 10 * v + (str_char_at(*str, n) - '0');
    n++;
  }

  if (n==0)
    return false;

  str_trim_left(str, n);
  *result = v;
  return true;
}

// Allocate a c-string and copy the contents of str into it
char* str_to_cstr(StringView str) {
  char* dest = malloc(sizeof(char) * (str.count + 1));
  memcpy(dest, str.data, str.count);
  dest[str.count] = '\0';

  return dest;
}

// Get char at index i of str
char str_char_at(StringView str, int i) {
  assert(i >= 0 && i < str.count);

  return str.data[i];
}
