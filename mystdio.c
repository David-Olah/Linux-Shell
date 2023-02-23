/*
  mystdio.c
  My Standard I/O

  Contains several i/o related helper functions.

  void print(char* str)
    Prints the string str to the standard output.
    str is a null-terminated string.

  void error(char* str)
    Prints the string str to the error stream.
    str is a null-terminated string.

  void read_input(char* buff, int max_size)
    Reads input from the standard input into buff until a newline is read,
    or max_size - 1 bytes have been read.
    A null terminator is added to the end of the buffer before returning.

  void print_raw(char* str, int size)
    Prints the string str to the standard output.
    str is NOT a null-terminated string.
    print_raw will print all of the memory contents in [str, str+size-1).
 */

#include <unistd.h>
#include "mystdio.h"
#include "myutil.h"

void output(char* str, int fd);

void print(char* str) {
  output(str, STDOUT_FILENO);
}

void error(char* str) {
  output(str, STDERR_FILENO);
}

void read_input(char* buff, int max_size) {
  int i = 0;
  char next;

  max_size--;

  // Read one character from STDIN at a time, adding it to the buffer.
  // Stop when max_size - 1 characters have been read.
  while (i < max_size) {
    read(STDIN_FILENO, &next, 1);
    if (next == '\n')
      break;
    else
      buff[i++] = next;
  }

  while (next != '\n') {
    read(STDIN_FILENO, &next, 1);
  }

  // Set the last character to a null terminator.
  buff[i] = '\0';
}

void print_raw(char* str, int size) {
  for (int i = 0; i < size; i++) {
    char next = str[i];
    if (next == '\0')
      write(STDOUT_FILENO, "\\0", 2);
    else
      write(STDOUT_FILENO, str + i, 1);
  }
}

void output(char* str, int fd) {
  int len = str_len(str);

  // Loop until all the bytes have been written to the file.
  do {
    int written = write(fd, str, len);
    if (written == -1)
      return;

    len -= written; // Decrease the remaining number of bytes to write.
    str += written; // Move the string pointer up to the un-written bytes.
  } while (len > 0);
}
