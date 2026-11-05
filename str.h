#ifndef STR_H
#define STR_H

#ifdef __cplusplus
  #define STRDEF extern "C"
#else
  #define STRDEF extern
#endif

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#ifndef STR_String_defined
#define STR_String_defined
typedef struct String {
  char *data;
  size_t count;
  size_t capacity;
} String;
#endif

#ifndef STR_StringView_defined
#define STR_StringView_defined
typedef struct StringView {
  const char *data;
  size_t count;
} StringView;
#endif

#ifndef STR_StringBounds_defined
#define STR_StringBounds_defined
typedef struct StringBounds {
  size_t start, end;
} StringBounds;
#endif

// Do not use these macros directly.
#define SV_impl(buf, size) (StringView){buf, size}
#define SV1(buf)        SV_impl((buf), strlen(buf))
#define SV2(buf, size)  SV_impl((buf), (size))
#define _SV_GET_MACRO(_1, _2, NAME, ...) NAME

/*
  SV(const char *buf, size_t size = strlen(buf))
  This macro creates string view depending on buf that you provided
  If you don't provide any size, it automatically calls strlen to get size
  Use SVL(literal) for creating string view from string literals.
*/
#define SV(...) _SV_GET_MACRO(__VA_ARGS__, SV2, SV1)(__VA_ARGS__)
#define SVS(str) SV(str.data, str.count)
#define SVL(str_lit) (StringView){str_lit, sizeof(str_lit)-1}

/*
  STR(const char *buf, size_t size = strlen(buf))
  This macro creates dynamic string from given buffer with size.
  Use STRL for string literals.
*/
#define STR(...) str_from_sv(SV(__VA_ARGS__))
#define STRL(str_lit) str_from_sv(SVL(str_lit))

/*
  Use SV_FMT and SV_ARG when printing string view via printf
  String literals can be concatenated while compile-time in C.
  So you can use SV_FMT like this: "sv = "SV_FMT"\n"
  SV_ARG also spreads count and data in format which printf wants

  You can use these when printing dynamic Strings btw.
  Or use STR_FMT and STR_ARG they're identical.
*/
#define SV_ARG(sv) (int)sv.count, sv.data
#define SV_FMT "%.*s"
#define STR_FMT SV_FMT
#define STR_ARG SV_ARG

// Dynamic strings functions
STRDEF String str_new(size_t init_capacity);
STRDEF String str_from_sv(StringView sv);
#define str_from str_from_sv
STRDEF bool str_reserve(String *self, size_t extra);
STRDEF bool str_append(String *self, char c);
STRDEF bool str_concat(String *self, StringView sv);
STRDEF void str_free(String *self);

// String view functions
STRDEF StringView sv_substr(String str, size_t start, size_t end);

STRDEF StringView sv_chop_left(StringView self, size_t count);
#define sv_remove_prefix sv_chop_left

STRDEF StringView sv_chop_right(StringView self, size_t count);
#define sv_remove_suffix sv_chop_right

STRDEF StringView sv_chop_until_char(StringView *self, char delim);
#define sv_chop_by_delim sv_chop_by_char
#define sv_chop_by_char sv_chop_until_char
#define sv_chop_until_delim sv_chop_until_char

STRDEF StringView sv_chop_by_func(StringView *self, int (*func)(int));

STRDEF void sv_trim_left(StringView *self);
STRDEF void sv_trim_right(StringView *self);
STRDEF void sv_trim(StringView *self);

/* If the functions that returns String or StringView
   has returned one of these, it means there's an error. */
static const String STR_INVALID = {.data=(char*)0xCCCCCCCC};
static const StringView SV_INVALID = {.data=(const char*)0xCCCCCCCC};

// You can check errors with these helper macros
#define STR_ISINVALID(str) (str.data == STR_INVALID.data)
#define SV_ISINVALID(sv) (sv.data == SV_INVALID.data)

#ifdef STR_IMPLEMENTATION
#include <stdlib.h>
#include <ctype.h>

String str_new(size_t init_capacity)
{
  String res = {0};
  if (!str_reserve(&res, init_capacity)) return STR_INVALID;
  return res;
}

String str_from_sv(StringView sv)
{
  String res = {0};
  if (!str_reserve(&res, sv.count + 1)) return STR_INVALID;
  memcpy(res.data, sv.data, sv.count);
  res.count = sv.count;
  res.data[res.count] = '\0';
  return res;
}

bool str_reserve(String *self, size_t extra)
{
  if (!self) return false;

  if (extra + self->count >= self->capacity) {
    size_t new_cap;
    if (self->capacity == 0) new_cap = extra + self->count;
    else new_cap = self->capacity * 2;
    new_cap += 1; // Space for '\0'

    // realloc(NULL, n) is equivalent to malloc(n), guarenteed in ISO C11 §7.22.3.5
    void *tmp = realloc(self->data, new_cap);
    if (!tmp) return false;
    self->data = tmp;
    self->capacity = new_cap;
  }
  return true;
}

bool str_append(String *self, char c)
{
  if (!str_reserve(self, 1)) return false;
  self->data[self->count++] = c;
  return true;
}

bool str_concat(String *self, StringView sv)
{
  if (!str_reserve(self, sv.count)) return false;
  memcpy(self->data + self->count, sv.data, sv.count);
  self->count += sv.count;
  self->data[self->count] = '\0';
  return true;
}

void str_free(String *self)
{
  if (!self) return;
  // free(NULL) is no-op, guarenteed in ISO C11 §7.22.3.3
  free(self->data);
  *self = (String){0};
}

StringView sv_substr(String str, size_t start, size_t end)
{
  if (start > end ||
      start > str.count ||
      end > str.count) return SV_INVALID;
  return (StringView){str.data + start, end - start};
}

StringView sv_chop_left(StringView self, size_t count)
{
  if (self.count < count) return SV_INVALID;
  return (StringView){.data=self.data + count, .count=self.count - count};
}

StringView sv_chop_right(StringView self, size_t count)
{
  if (self.count < count) return SV_INVALID;
  return (StringView){.data=self.data, .count=self.count - count};
}

StringView sv_chop_until_char(StringView *self, char delim)
{
  if (!self) return SV_INVALID;

  const char *found = memchr(self->data, delim, self->count);
  if (!found) {
    StringView result = *self;
    self->data = NULL;
    self->count = 0;
    return result;
  }

  size_t n = found - self->data;
  StringView result = {.data=self->data, .count=n};

  self->count -= n + 1;
  if (self->count == 0) self->data = NULL;
  else self->data = found + 1;
  return result;
}

StringView sv_chop_by_func(StringView *self, int (*func)(int))
{
  if (!self) return SV_INVALID;

  for (size_t i = 0; i < self->count; i++) {
    char c = self->data[i];
    if (!func((unsigned char)c)) continue;
    // found
    StringView result = {.data=self->data, .count=i};
    self->count -= i + 1;
    if (self->count == 0) self->data = NULL;
    else self->data += i + 1;
    return result;
  }

  // not found
  StringView result = *self;
  self->data = NULL;
  self->count = 0;
  return result;
}

void sv_trim_left(StringView *self)
{
  if (!self || self->count == 0) return;
  size_t leading_spaces = 0;
  for (const char *c = self->data; c < self->data + self->count; c++) {
    if (isspace((unsigned char)(*c))) leading_spaces += 1;
    else break;
  }

  self->count -= leading_spaces;
  if (self->count == 0) self->data = NULL;
  else self->data += leading_spaces;
}

void sv_trim_right(StringView *self)
{
  if (!self || self->count == 0) return;
  size_t trailing_spaces = 0;
  for (const char *c = self->data + self->count - 1; c >= self->data; c--) {
    if (isspace((unsigned char)(*c))) trailing_spaces += 1;
    else break;
  }
  self->count -= trailing_spaces;
  if (self->count == 0) self->data = NULL;
}

void sv_trim(StringView *self)
{
  sv_trim_left(self);
  sv_trim_right(self);
}

#endif // STR_IMPLEMENTATION

#endif // STR_H

/*
  The MIT License
  Copyright (c) 2026 ilpeN

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/
