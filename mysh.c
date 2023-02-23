/*
  mysh.c
  My Shell

  Custom shell program.
 */

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>

#include "mystdio.h"
#include "myutil.h"
#include "mystdlib.h"
#include "mysh_commands.h"

#define IN_BUFF_SIZE 256  // Size of input buffer.
#define DIR_BUFF_SIZE 256 // Size of file directory buffer.
#define MAX_ARGC 10       // Maximum number of arguments per clause.
#define MAX_CLAUSES 10    // Maximum number of clauses per line.

// For pipes
#define READ_END 0
#define WRITE_END 1

// Struct representing a line of input to the shell.
typedef struct {
  // Token list used during parsing process.
  // Size accomodates 10 clauses of 10 arguments + null-terminator each,
  // two entries for I/O redirection files, and one for optional ampersand.
  // Token count tracks how many tokens are stored in the list.
  char* token_list[MAX_CLAUSES * (MAX_ARGC + 1) + 3];
  int token_count;

  // Clause list that stores the list of executable programs in the line.
  // Size accomodates 10 clauses of 10 arguments + null-terminator each.
  // Clause count tracks how many clauses are stored in the list.
  char* clause_list[MAX_CLAUSES][MAX_ARGC + 1];
  char* executable_paths[MAX_CLAUSES];
  int clause_count;

  // run_bg is a flag that indicates if the line should be executed in background mode.
  int run_bg;

  // input_file and output_file are null-terminated strings holding the directories of
  // the I/O redirection files. If no redirection file was specified for input or output,
  // the corresponding pointer will be NULL.
  char* input_file;
  char* output_file;
} Line;

void show_prompt();
int parse_line(Line* line);
int parse_tokens(char* input_buffer, Line* line);
int get_executable_paths(Line* line);
int check_file_access(Line* line);
int run_line(Line* line);

extern char** environ;
char dir_buffer[DIR_BUFF_SIZE];
int login_mode = 0;

int main(int argc, char* argv[]) {
  char input_buffer[IN_BUFF_SIZE];
  Line line;

  // Handle login shell. Set directory to / and print login message.
  if (argv[0][0] == '-') {
    char* cd_command[] = {"cd", "\0/"};
    handle_command(2, cd_command);
    print("Welcome, user.\nYou are using MyShell™!\n");
    login_mode = 1;
  }

  // Main loop; repeatedly asks for input and attempts to execute it.
  while (1) {
    // Read a line of input from STDIN.
    show_prompt();
    read_input(input_buffer, IN_BUFF_SIZE);
    
    // Parse input into tokens
    if (!parse_tokens(input_buffer, &line)) {
      error("Invalid command. Check syntax.\n");
      continue;
    }
    
    // Check for internal commands.
    if (handle_command(line.token_count, line.token_list))
      continue;
    
    // Parse input for execution.
    if (!parse_line(&line)) {
      error("Invalid command. Check syntax.\n");
      continue;
    }
    
    // Get executable files from $PATH.
    if (!get_executable_paths(&line))
      continue;
    
    // Check files for relevant permissions.
    if (!check_file_access(&line))
      continue;
    
    // Attempt to run the line.
    run_line(&line);
  }
  
  return 0;
}

// Displays the prompt for a line of input.
void show_prompt() {
  char* username = get_env("USER");
  char* pwd = get_env("PWD");
  char* home = get_env("HOME");
  int home_len = home != NULL ? str_len(home) : 0;
  char hostname[HOST_NAME_MAX + 1];
  gethostname(hostname, HOST_NAME_MAX + 1);
  print(username);
  print("@");
  print(hostname);
  print(":");
  
  if (home != NULL && str_eql_t(home, pwd, home_len)) {
    pwd += home_len;
    print("~");
  }
  
  print(pwd);
  print("$ ");
}

// Parses the given input buffer into the given line struct.
int parse_line(Line* line) {
  // Initialize some values.
  int argument_count = 0;
  char** clause = line->clause_list[0];
  int in_redirect = 0;
  
  line->clause_count = 0;
  line->run_bg = 0;
  line->input_file = NULL;
  line->output_file = NULL;

  // Loop through all the tokens.
  for (int i = 0; i < line->token_count; i++) {
    char* token = line->token_list[i];
    char operator = '\0';
    // The first token has no operator prefix.
    // The rest do, so save it, change it to null in the buffer, and advance to the string.
    if (i > 0) {
      operator = token[0];
      token[0] = '\0';
      token++;
    }

    switch(operator) {
      // If the token has no operator, just add it to the current clause.
    case '\0':
      if (in_redirect)
	return 0;
      if (argument_count > MAX_ARGC)
	return 0;
      clause[argument_count++] = token;
      break;

      // If the token has a pipe operator, move to next clause and add argument to it.
    case '|':
      if (line->clause_count >= MAX_CLAUSES - 1)
	return 0;
      clause[argument_count] = NULL;
      argument_count = 0;
      clause = line->clause_list[++line->clause_count];
      clause[argument_count++] = token;
      in_redirect = 0;
      break;

      // If the token has an input redirection operator, set as input file.
    case '<':
      if (line->input_file != NULL)
	return 0;
      line->input_file = token;
      in_redirect = 1;
      break;

      // If the token has an output redirection operator, set as output file.
    case '>':
      if (line->output_file != NULL)
	return 0;
      line->output_file = token;
      in_redirect = 1;
      break;
    }
  }

  // Check the last character of the last argument for ampersand.
  // If so, enable background mode and replace it with null-terminator.
  // If it's the only character in the argument, reduce the argument count by 1.
  // If ^ and it's the only argument in the clause, there is a syntax error.
  char* last_argument = clause[argument_count - 1];
  int last_argument_len = str_len(last_argument);
  if (last_argument[last_argument_len - 1] == '&') {
    line->run_bg = 1;
    last_argument[last_argument_len - 1] = '\0';
    if (last_argument_len == 1) {
      if (argument_count-- == 1)
	return 0;
    }
  }

  // Null-terminate the current clause and increment the clause count.
  clause[argument_count] = NULL;
  line->clause_count++;
  
  return 1;
}

// Parses the input buffer into a list of tokens. Tokens are prefixed with their
// operator character if they have one. Otherwise, they are prefixed with \0.
int parse_tokens(char* input_buffer, Line* line) {
  int in_word = 0;        // Flag indicating if the loop is in a word/token.
  int in_operator = 0;    // Flag indicating if the loop is in an operator.
  char operator = '\0';   // Holds the current operator symbol or \0 if none.
  line->token_count = 0;

  // Loop through every character.
  for (int i = 0; i < IN_BUFF_SIZE; i++) {
    char next = input_buffer[i];

    switch(next) {
      // Case where next is an operator symbol.
    case '|':
    case '<':
    case '>':
      // If there is already an operator or there are no tokens yet, there is invalid syntax.
      if (in_operator || line->token_count == 0)
	return 0;
      in_operator = 1;
      operator = next;
      
      // Case where next is an operator symbol or a space.
    case ' ':
      input_buffer[i] = '\0'; // Replace symbol with null to ensure tokens are null-terminated.
      in_word = 0;
      break;

      // Case where next is anything else.
    default:
      // If loop is not in a word, mark this as the next token.
      if (!in_word) {
	// If this is not the first token, save the operator symbol as a prefix.
	if (line->token_count > 0) {
	  line->token_list[line->token_count++] = input_buffer + i - 1;
	  input_buffer[i - 1] = operator;
	} else {
	  line->token_list[line->token_count++] = input_buffer + i;
	}
      }

      in_word = 1;
      in_operator = 0;
      operator = '\0';
      break;
    }
  }

  // If the loop exits in an operator, there is invalid syntax.
  if (in_operator)
    return 0;
  
  return 1;
}

// Checks the paths stored in the clauses and tries to find them in $PATH if needed.
int get_executable_paths(Line* line) {
  char* buff_trav = dir_buffer;

  // Loop through every clause.
  for (int i = 0; i < line->clause_count; i++) {
    // Get the file directory.
    char* file_dir = line->clause_list[i][0];

    // If the file directory exists, save it as-is.
    if (access(file_dir, F_OK) == 0) {
      line->executable_paths[i] = file_dir;
      continue;
    }

    int found_path = 0;
    // Ensure the directory does not start with a / or .
    if (file_dir[0] != '/' && file_dir[0] != '.') {
      found_path = find_in_path(file_dir, buff_trav);
    }

    // If a path was found, add it to the list of executable paths.
    if (found_path) {
      line->executable_paths[i] = buff_trav;
      buff_trav += str_len(buff_trav) + 1;
    } else {
      // If a path was not found, print error message and return.
      error("File ");
      error(file_dir);
      error(" not found.\n");
      return 0;
    }
  }
  return 1;
}

int check_file_access(Line* line) {
  // Check that the files exist and are executable.
  for (int i = 0; i < line->clause_count; i++) {
    char* file_dir = line->executable_paths[i];
    if (access(file_dir, X_OK) != 0) {
      error("File ");
      error(file_dir);
      error(" missing executable access.\n");
      return 0;
    }
  }

  // Check that the input file exists and is readable.
  if (line->input_file != NULL) {
    char* file_dir = line->input_file;
    if (access(file_dir, F_OK) != 0) {
      error("File ");
      error(file_dir);
      error(" not found.\n");
      return 0;
    } else if (access(file_dir, R_OK) != 0) {
      error("File ");
      error(file_dir);
      error(" missing read access.\n");
      return 0;
    }
  }

  // Check that the output file is writable.
  if (line->output_file != NULL) {
    char* file_dir = line->output_file;
    if (access(file_dir, F_OK) == 0 && access(file_dir, W_OK) != 0) {
      error("File ");
      error(file_dir);
      error(" missing write access.\n");
      return 0;
    }
  }

  return 1;
}

// Runs the given line.
int run_line(Line* line) {
  char** envp = environ;
  int pipes[MAX_CLAUSES - 1][2];
  int pids[MAX_CLAUSES - 1];

  // Prints out all the clauses and arguments. Meant for debugging.
  /*
  for (int i = 0; i < line->clause_count; i++) {
    printf("Clause #%d:\n", i);
    char** arguments = line->clause_list[i];
    int j = 0;
    while (arguments[j] != NULL) {
      printf("  Argument #%d: %s\n", j, arguments[j]);
      j++;
    }
  }
  */

  // Loop through every clause.
  for (int i = 0; i < line->clause_count; i++) {
    char** arguments = line->clause_list[i];
    char* file_path = line->executable_paths[i];
    int first_clause = i == 0;
    int last_clause = i == line->clause_count - 1;

    // Create a pipe if not the last clause.
    if (!last_clause)
      pipe(pipes[i]);

    // Fork.
    int pid = fork();
    if (pid == -1) {
      error("Failed to fork process.\n");
      return 0;
    }

    // Child process:
    if (pid == 0) {
      // If not the last clause, close read end of pipe and point STDOUT to the write end.
      if (!last_clause) {
	close(pipes[i][READ_END]);
        dup2(pipes[i][WRITE_END], STDOUT_FILENO);
      } else if (line->output_file != NULL) {
	// If it is the last clause and there is an output redirection, connect it.
	int fd = open(line->output_file, O_WRONLY | O_CREAT, 0666);
	dup2(fd, STDOUT_FILENO);
      }
      // If not the first clause, close write end of previous pipe, point STDIN to the read end.
      if (!first_clause) {
	close(pipes[i - 1][WRITE_END]);
	dup2(pipes[i - 1][READ_END], STDIN_FILENO);
      } else if (line->input_file != NULL) {
	// If it is the first clause and there is an input redirection, connect it.
	int fd = open(line->input_file, O_RDONLY);
	dup2(fd, STDIN_FILENO);
      }

      // Execute the program.
      execve(file_path, arguments, envp);
      _exit(0); // Terminate process if execve fails.
    } else {
      pids[i] = pid;
    }

    // If not the last clause, close the write end of the pipe.
    if (!last_clause)
      close(pipes[i][WRITE_END]);
    // If not the first clause, close the read end of the pipe.
    if (!first_clause)
      close(pipes[i - 1][READ_END]);
  }

  // Wait for all child processes to terminate if not running in background mode.
  if (!line->run_bg) {
    for (int i = 0; i < line->clause_count; i++)
      waitpid(pids[i], NULL, 0);
  }

  return 1;
}
