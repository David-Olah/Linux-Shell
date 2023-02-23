/*
  myutil.c
  My Util

  Contains miscellaneous helper functions.

  int str_eql(char* str1, char* str2)
    Returns 1 if str1 and str2 have equal contents.
    Otherwise returns 0.
    str1 and str2 are null-terminated strings.

    int str_len(char* str)
    Returns the number of bytes in str, not including the null-terminator.
    str is a null-terminated string.
 */

#include <unistd.h>

#include "myutil.h"
#include "mystdlib.h"

int str_eql(const char* str1, const char* str2) {
  return str_eql_t(str1, str2, '\0');
}

int str_eql_t(const char* str1, const char* str2, char terminator) {
  int i = 0;
  while (str1[i] == str2[i] && str1[i] != terminator)
    i++;
  return str1[i] == str2[i];
}

int str_eql_l(const char* str1, const char* str2, int len) {
  for (int i = 0; i < len; i++)
    if (str1[i] != str2[i])
      return 0;
  return 1;
}

int str_len(const char* str) {
  return str_len_t(str, '\0');
}

int str_len_t(const char* str, char terminator) {
  int len = 0;
  while (str[len] != terminator) len++;
  return len;
}

char* str_chr(const char* str, char c) {
  while (*str != '\0' && *str != c) {
    str++;
  }
  return (*str == '\0') ? NULL : str;
}

void str_copy(const char* source, char* dest, int len) {
  for (int i = 0; i < len; i++)
    dest[i] = source[i];
}

void str_cat(const char* source, char* dest) {
  while (*(dest++) != '\0');
  while ((*(dest++) = *(source++)) != '\0');
}

int str_to_int(const char* str, int* num) {
  int x = 0;
  int negative = str[0] == '-';
  if (negative) {
    str++;
  }
  
  if (*str == '\0')
    return 0;

  char next;
  while ((next = *str++) != '\0') {
    if (next < '0' || next > '9')
      return 0;
    x *= 10;
    x += next - '0';
  }

  if (negative)
    x = -x;

  *num = x;
  return 1;
}

int find_in_path(const char* dir, char* out_buffer) {
  int dir_len = str_len(dir);
  // Get $PATH.
  char* path = get_env("PATH");
  char* next_colon;
  // Loop through the comma-separated paths.
  do {
    next_colon = str_chr(path, ':');

    // Get the length of the current path.
    int len = next_colon == NULL ? str_len(path) : next_colon - path;
    // Copy the path into the output buffer, add a /, add the file dir, and null-terminate it.
    str_copy(path, out_buffer, len);
    *(out_buffer + len) = '/';
    str_copy(dir, out_buffer + len + 1, dir_len);
    *(out_buffer + len + dir_len + 1) = '\0';

    // If it's a valid path, return it.
    if (access(out_buffer, F_OK) == 0)
      return 1;

    // Advance to the next path.
    path = next_colon + 1;
  } while (next_colon != NULL);

  return 0;
}
