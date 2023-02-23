#include <stdio.h>
#include <unistd.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <errno.h>

#include "mysh_commands.h"
#include "myutil.h"
#include "mystdio.h"
#include "mystdlib.h"

void shell_error(char* str);
void __exit(int argc, char** argv);
void cd(int argc, char** argv);

extern char** environ;
char* current_command = NULL;

void* commands[] = {
  "exit", &__exit,
  "logout", &__exit,
  "cd", &cd,
  NULL
};

void shell_error(char* str) {
  error("-mysh: ");
  if (current_command != NULL) {
    error(current_command);
    error(": ");
  }
  error(str);
  error("\n");
}
  
int handle_command(int argc, char** argv) {
  int i;
  for (i = 1; i < argc; i++)
    argv[i]++;
  
  i = 0;
  while (commands[i] != NULL) {
    if (str_eql(commands[i], argv[0])) {
      current_command = commands[i];
      void (*command)(int, char**);
      command = commands[i + 1];
      (*command)(argc, argv);
      current_command = NULL;
      return 1;
    }
    i += 2;
  }

  for (i = 1; i < argc; i++)
    argv[i]--;
  
  return 0;
}

void __exit(int argc, char** argv) {
  if (argc > 2) {
    shell_error("too many arguments");
    return;
  }

  int status = 0;
  if (argc == 2) {
    str_to_int(argv[1], &status);
  }
  
  _exit(status);
}

void cd(int argc, char** argv) {
  if (argc > 2) {
    shell_error("too many arguments");
    return;
  }

  int result = argc == 1 ? chdir("~") : chdir(argv[1]);
  if (result == 0) {
    char cwd[PATH_MAX + 1];
    getcwd(cwd, PATH_MAX);
    setenv("PWD", cwd, 1);
  } else {
    switch(errno) {
    case(EACCES):
      shell_error("Permission denied");
      break;
    case(ENOENT):
      shell_error("No such file or directory");
      break;
    default:
      shell_error("Something went wrong");
      break;
    }
  }
}
