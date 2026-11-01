#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>

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
  size_t i = 0;
  while (i < sv->count && istype(sv->data[i])) {
      i += 1;
  }

  size_t j = sv->count - 1;
  while (j >= 0 && istype(sv->data[j])) {
    j--;
  }

  if (i >= j) {
    return;
  }

  if (j - i + 1 >= 0) {
    sv->count = j - i + 1;
    sv->data += i;
  }
}

void str_trim_by_delim(StringView *sv, char c) {
  size_t i = 0;
  while (i < sv->count && sv->data[i] == c) {
      i += 1;
  }

  size_t j = sv->count - 1;
  while (j >= 0 && sv->data[j] == c) {
    j--;
  }

  if (i >= j) {
    return;
  }

  if (j - i + 1 >= 0) {
    sv->count = j - i + 1;
    sv->data += i;
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

int is_quote(int c) {
  return (c == '\"' || c == '\'');
}

#define strfmt "%.*s"
#define strarg(s) (s).count, (s).data

void test() {
  StringView str = str_of_cstr("\"   hello  \"");
  printf("Before: |"strfmt"|\n", strarg(str));

  str_trim_by_predicate(&str, is_quote);
  str_trim_by_predicate(&str, isspace);
  printf("After: |"strfmt"|\n", strarg(str));

  str = str_of_cstr("12345");
  printf("Before: |"strfmt"|\n", strarg(str));
  str_trim_left(&str, 2);
  str_trim_right(&str, 2);
  printf("After: |"strfmt"|\n", strarg(str));
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
