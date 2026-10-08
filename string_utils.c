#include "string_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *string_copy(const char *value) {
  size_t length = strlen(value) + 1;
  char *copy = malloc(length);
  if (copy != NULL) {
    memcpy(copy, value, length);
  }
  return copy;
}

char *string_replace(const char *source, const char *from, const char *to) {
  size_t from_len = strlen(from);
  size_t to_len = strlen(to);
  size_t count = 0;

  for (const char *p = strstr(source, from); p != NULL;
       p = strstr(p + from_len, from)) {
    count++;
  }

  char *result = malloc(strlen(source) + count * (to_len > from_len ? to_len - from_len : 0) + 1);
  if (result == NULL) {
    return NULL;
  }

  char *out = result;
  const char *cursor = source;
  for (const char *p = strstr(cursor, from); p != NULL;
       p = strstr(cursor, from)) {
    size_t chunk = (size_t)(p - cursor);
    memcpy(out, cursor, chunk);
    out += chunk;
    memcpy(out, to, to_len);
    out += to_len;
    cursor = p + from_len;
  }
  strcpy(out, cursor);
  return result;
}

size_t utf8_length(const char *text) {
  size_t length = 0;
  for (; *text != '\0'; text++) {
    if (((unsigned char)*text & 0xC0) != 0x80) {
      length++;
    }
  }
  return length;
}

void table_print_cell(const char *text, size_t width) {
  printf("| %s", text);
  for (size_t i = utf8_length(text); i < width; i++) {
    putchar(' ');
  }
  putchar(' ');
}

void table_print_rule(const size_t *widths, int columns) {
  for (int i = 0; i < columns; i++) {
    putchar('+');
    for (size_t j = 0; j < widths[i] + 2; j++) {
      putchar('-');
    }
  }
  puts("+");
}
