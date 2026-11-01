#include <string.h>

typedef struct {
  const char* data;
  size_t count;
} StringView;

int is_quote(int c);
StringView str_of_cstr(const char *cstr);
StringView str_split_by_delim(StringView *sv, char delim);
StringView str_split_by_predicate(StringView *sv, int (*istype)(int c));
void str_trim_by_delim(StringView *sv, char c);
void str_trim_by_predicate(StringView *sv, int (*istype)(int c));
void str_trim_right(StringView *sv, size_t n);
void str_trim_left(StringView *sv, size_t n);
bool str_starts_with(StringView a, StringView b);
bool str_ends_with(StringView a, StringView b);
