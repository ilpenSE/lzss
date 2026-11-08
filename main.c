#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#define BW_IMPL
#include "bw.h"
#define STR_IMPLEMENTATION
#include "str.h"

#define WINDOW 4096 // Max distance
#define MIN_MATCH 3
#define MAX_MATCH 258

typedef struct {
  bool is_match;
  union {
    unsigned char lit;
    struct {
      uint16_t dist;
      uint8_t len;
    };
  } as;
} Token;

typedef struct {
  size_t length;
  size_t distance;
} LenDistPair;

LenDistPair find_match_kmp(const char *buf, size_t size, size_t i)
{
  size_t m = size - i < MAX_MATCH ? size - i : MAX_MATCH;
  if (m < MIN_MATCH || i == 0) return (LenDistPair){0, 0};

  const char *pat = buf + i;
  size_t kmp_table[MAX_MATCH] = {0};
  for (size_t q = 1, k = 0; q < m; q++) {
    while (k > 0 && pat[k] != pat[q]) k = kmp_table[k - 1];
    if (pat[k] == pat[q]) k++;
    kmp_table[q] = k;
  }

  size_t start = i > WINDOW ? i - WINDOW : 0;
  size_t end = i + m - 1;
  size_t best_l = 0, best_d = 0;
  for (size_t j = start, k = 0; j < end; j++) {
    while (k > 0 && (k == m || pat[k] != buf[j])) k = kmp_table[k - 1];
    if (pat[k] == buf[j]) k++;

    size_t t = j + 1 - k;
    if (t < i && k > best_l) { best_l = k; best_d = i - t; }
    if (k == m) break;
  }

  return (LenDistPair){.length=best_l, .distance=best_d};
}

LenDistPair find_match_brute_force(const char *buf, size_t size, size_t i)
{
  size_t best_l = 0, best_d = 0;
  size_t start = i > WINDOW ? i - WINDOW : 0;
  for (size_t j = start; j < i; j++) {
    size_t k = 0;
    while (k < MAX_MATCH && i + k < size && buf[j + k] == buf[i + k]) k++;
    if (k > best_l) { best_l = k; best_d = i - j; }
  }
  return (LenDistPair){.length=best_l, .distance=best_d};
}

String lzss_encode(const char *input, size_t size, LenDistPair (*find_match_fn)(const char *, size_t, size_t))
{
  BitWriter bw = bw_new(32);
  size_t i = 0;
  while (i < size) {
    LenDistPair p = find_match_fn(input, size, i);
    size_t best_l = p.length, best_d = p.distance;

    if (best_l >= MIN_MATCH) {
      bw_put(&bw, 1);
      bw_puts(&bw, (uint8_t)(best_l - 3), 8);
      bw_puts(&bw, (uint16_t)best_d, 16);
      i += best_l;
    } else {
      bw_put(&bw, 0);
      bw_puts(&bw, (unsigned char)input[i], 8);
      i++;
    }
  }

  return (String){.data=(char *)bw.buf, .count=bw_count(&bw), .capacity=bw.capacity};
}

String lzss_decode(const char *buffer, size_t size)
{
  String str = {0};
  size_t i = 0;

  BitReader br = {.buf=(const bw_byte_t*)buffer, .pos=size*8};
  while (i + 9 <= br.pos) {
    if (br_get(&br, i++)) {
      uint16_t len = br_gets(&br, i, 8) + 3;
      uint16_t dist = br_gets(&br, i + 8, 16);
      for (size_t j = 0; j < len; j++) {
        str_append(&str, (char)str.data[str.count - dist]);
      }
      i += 24;
    } else {
      char b = (char)br_gets(&br, i, 8);
      str_append(&str, b);
      i += 8;
    }
  }
  str.data[str.count] = '\0';
  return str;
}

size_t write_entire_file(StringView sv, const char *file_name);
String read_entire_file(const char *file_name);

bool codec_file(const char *input_file, const char *output_file, bool encode);

bool encode_file(const char *input_file, const char *output_file)
{ return codec_file(input_file, output_file, true); }

bool decode_file(const char *input_file, const char *output_file)
{ return codec_file(input_file, output_file, false); }

bool codec_file(const char *input_file, const char *output_file_, bool encode)
{
  String content = read_entire_file(input_file);
  if (STR_ISINVALID(content)) return false;

  String output;
  if (encode) {
    output = lzss_encode(content.data, content.count, &find_match_brute_force);
  } else {
    output = lzss_decode(content.data, content.count);
  }
  if (STR_ISINVALID(output)) return false;
  size_t orig_bytes = content.count;
  str_free(&content);

  char output_file[4096] = {0};
  if (output_file_) {
    size_t output_file__count = strlen(output_file_);
    if (output_file__count >= sizeof(output_file)) output_file__count = sizeof(output_file) - 1;
    memcpy(output_file, output_file_, output_file__count);
    output_file[output_file__count] = '\0';
  } else {
    snprintf(output_file, sizeof(output_file), "%s.lzss_%s", input_file, (encode ? "encoded" : "decoded"));
  }

  size_t written = write_entire_file(SVS(output), output_file);
  printf("%s (%zu bytes) %s into %s (%zu bytes).\n",
         input_file, orig_bytes, (encode ? "encoded" : "decoded"), output_file, written);
  str_free(&output);
  return true;
}

const char *shift_argv(int *argc, char ***argv)
{ return *argc == 0 ? NULL : ((*argc)--, *(*argv)++); }

void print_usage(const char *program_name, FILE *f)
{
  fprintf(f, "Usage: %s <input file> [...OPTIONS]\n", program_name);
  fprintf(f, "Options:\n");
  fprintf(f, "    -e | --encode: Do encoding (deflating) (By default).\n");
  fprintf(f, "    -d | --decode: Do decoding (inflating).\n");
  fprintf(f, "    -o | --output <output_file>: Specify output file of program.\n");
  fprintf(f, "        Optional, program'll choose a name suffixed with lzss_encoded or lzss_decoded depending on operation\n");
  fprintf(f, "    -h | --help: Print this help message.\n");
}

int main(int argc, char **argv)
{
  const char *program_name = shift_argv(&argc, &argv);
  const char *input_file = NULL; //  @NonNULL
  const char *output_file = NULL; // @NULLable
  bool encode = true;

  const char *arg;
  while ((arg = shift_argv(&argc, &argv)) != NULL) {
    if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
      print_usage(program_name, stdout);
      return 0;
    } else if (strcmp(arg, "--output") == 0 || strcmp(arg, "-o") == 0) {
      output_file = shift_argv(&argc, &argv);
      if (!output_file) {
        fprintf(stderr, "ERROR: Output file is required when '%s' is passed!\n", arg);
        fprintf(stderr, "       Hint: Use something like this: %s %s %s encoded.txt\n\n",
                program_name, (input_file ? input_file : "my_input.txt"), arg);
        print_usage(program_name, stderr);
        return 1;
      }
    } else if (strcmp(arg, "-d") == 0 || strcmp(arg, "--decode") == 0) {
      encode = false;
    } else if (strcmp(arg, "-e") == 0 || strcmp(arg, "--encode") == 0) {
      encode = true;
    } else if (*arg == '-') {
      fprintf(stderr, "ERROR: Unrecognized option: %s\n\n", arg);
      print_usage(program_name, stderr);
    } else {
      if (input_file) {
        fprintf(stderr, "ERROR: There can be only ONE input file. Original was %s\n", input_file);
        fprintf(stderr, "       Hint: Remove '%s' or '%s' from command line.\n\n", arg, input_file);
        print_usage(program_name, stderr);
        return 1;
      }
      input_file = arg;
    }
  }

  if (!input_file) {
    fprintf(stderr, "ERROR: Input file wasn't specified!\n");
    print_usage(program_name, stderr);
    return 1;
  }

  if (encode) {
    if (!encode_file(input_file, output_file)) return 1;
  } else {
    if (!decode_file(input_file, output_file)) return 1;
  }
}

String read_entire_file(const char *file_name)
{
  FILE *f = fopen(file_name, "rb");
  if (!f) return STR_INVALID;
  if (fseek(f, 0, SEEK_END) != 0) return STR_INVALID;
  long n = ftell(f);
  if (n < 0) return STR_INVALID;
  size_t file_size = (size_t)n;
  if (fseek(f, 0, SEEK_SET) != 0) return STR_INVALID;

  String res = str_new(file_size + 1);

  size_t bytes_read = fread(res.data, 1, file_size, f);
  if (bytes_read == 0) {
    if (feof(f) || ferror(f)) return STR_INVALID;
  }
  res.count = bytes_read;

  if (fclose(f) != 0) return STR_INVALID;
  return res;
}

size_t write_entire_file(StringView sv, const char *file_name)
{
  FILE *f = fopen(file_name, "wb");
  if (!f) return 0;
  size_t written = fwrite(sv.data, 1, sv.count, f);
  if (written != sv.count || ferror(f)) return 0;
  if (fclose(f) != 0) return 0;
  return written;
}
