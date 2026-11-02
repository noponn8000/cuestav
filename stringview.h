#include <string.h>

typedef struct {
  const char* data;
  size_t count;
} StringView;

StringView str_of_cstr(const char *cstr);
char* str_to_cstr(StringView str);
char str_char_at(StringView str, int i);
StringView str_split_by_delim(StringView *sv, char delim);
StringView str_split_by_predicate(StringView *sv, int (*istype)(int c));
void str_trim_by_delim(StringView *sv, char c);
void str_trim_by_predicate(StringView *sv, int (*istype)(int c));
void str_trim_right(StringView *sv, size_t n);
void str_trim_left(StringView *sv, size_t n);
bool str_starts_with(StringView a, StringView b);
bool str_ends_with(StringView a, StringView b);
bool str_parse_uint(StringView* str, int* result);
bool str_eat_prefix(StringView* str, StringView prefix);
bool str_eat_char(StringView* str, char c);

#define strfmt "%.*s"
#define strarg(s) (s).count, (s).data
#define SV(lit) (StringView) { .data = (lit), .count = sizeof(lit) - 1}
