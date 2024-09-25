// https://brennan.io/2015/01/16/write-a-shell-in-c/

#include "colors.h"
// #include "history.c"
// #include "history.h"
#include "history.h"
#include "read_line.c"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/_types/_ssize_t.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#define LSH_RL_BUFFSIZE 1024

char *color = YELLOW;
struct History *hist;
struct termios *term;

char *lsh_read_line(void) {
  char *line = NULL;
  size_t bufsize = 0; // have getline allocate a buffer for us

  if (getline(&line, &bufsize, stdin) == -1) {
    if (feof(stdin)) {
      exit(EXIT_SUCCESS);
    } else {
      perror("readLine");
      exit(EXIT_FAILURE);
    }
  }

  return line;
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

char **lsh_split_line(char *line) {
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char *));
  char *token;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation eror\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);

  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char *));
      if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
    token = strtok(
        NULL, LSH_TOK_DELIM); // strtok is called again with NULL as the first
                              // argument, which tells it to continue tokenizing
                              // the same line string from where it left off.
  }
  tokens[position] = NULL;
  return tokens;
}

// Function declarations for builtin shell commands:

int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);
int change_color(char **args);

// List of builtin commands, followed by their corresponding functions

char *builtin_str[] = {"cd", "help", "exit", "change-color"};

int (*builtin_func[])(char **) = {&lsh_cd, &lsh_help, &lsh_exit, &change_color};

int lsh_num_builtins() { return sizeof(builtin_str) / sizeof(char *); }

int lsh_cd(char **args) {
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }
  return 1;
}

void toUpper(char *str) {
  while (*str) {
    *str = toupper(*str);
    str++;
  }
}

int change_color(char **args) {
  // check if -h     check if NULL     check if ValidColor
  if (args[1] == NULL) {

    printf("lsh: change-color requires an argument, type 'change-color -h' for "
           "further information\n");

  } else if (strcmp(args[1], "-h") == 0) {

    printf("change-color usage: change-color option: \n");
    for (int i = 0; i < N_COLORS; i++) {
      printf("%s  ", COLOR_LIST[i]);
      if (i % 3 == 0) {
        printf("\n");
      }
    }
    printf("\n");

  } else {

    // 3 is the minimun lenght (RED)
    bool isvalid = false;
    char *input = args[1];
    toUpper(input);
    for (int i = 0; !isvalid && i < N_COLORS; i++) {
      if (strncmp(input, COLOR_LIST[i], 64) == 0) {
        color = (char *)COLOR_VALUE[i];
        isvalid = true;
      }
    }

    if (!isvalid) {
      printf("lsh: select a valid option, type 'change-color -h' for futher "
             "information\n");
    }
  }

  return 1;
}

int lsh_help(char **args) {
  /*int i;
  printf("Stephen Brenna's LSH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");*/

  printf("Stephen Brenna's & Pacho Ruiz LSH\n");
  printf("The following is an extended implementation of Brenna's shell, my "
         "implementation maintain the original\n");
  printf("base of the project, an lsh_loop with three steps: \n- Reading \n- "
         "Split the lines of the comand \n- Execute the comand\n\n");
  printf("The main change is the dynamic reading, this gives more oportunities "
         "to implement a functions triggered by certain keys. That lead to "
         "implement a custom\nalgorithm to save the input and manage the "
         "delation of key.\n\n");
  return 1;
}

int lsh_exit(char **args) { return 0; }

int lsh_launch(char **args) {
  pid_t pid, wpid;
  int status;

  pid = fork();
  if (pid == 0) {
    // child process
    if (execvp(args[0], args) == -1) {
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // error forking
    perror("lsh: error forking");
  } else {
    // parent process
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

int lsh_execute(char **args) {
  int i;

  if (args[0] == NULL) {
    return 1; // empty command
  }

  for (i = 0; i < lsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return lsh_launch(args);
}

char *lsh_current_dir() {
  FILE *ls_cmd = popen("pwd", "r");
  if (ls_cmd == NULL) {
    fprintf(stderr, "error (lsh_current_dir)(1)\n");
    exit(EXIT_FAILURE);
  }

  static char full_dir[1024];
  size_t n;

  while ((n = fread(full_dir, 1, sizeof(full_dir) - 1, ls_cmd)) > 0) {
    full_dir[n] = '\0';
  }

  int position = 0, bufsize = 64;
  char **dirs;
  char *dir;
  char *delim = "/";

  dirs = malloc(bufsize * sizeof(char *));
  dir = strtok(full_dir, delim);

  while (dir != NULL) {
    dirs[position++] = dir;

    if (position >= bufsize) {
      bufsize += 64;
      dirs = realloc(dirs, bufsize * sizeof(char *));
      if (dirs == NULL) {
        fprintf(stderr, "lsh_current_dir (ERROR: reallocation dirs)");
        exit(EXIT_FAILURE);
      }
    }

    dir = strtok(NULL, "/");
  }

  if (pclose(ls_cmd) < 0) {
    perror("error (lsh_current_dir)(2)\n");
  }

  char *current_dir = dirs[position - 1];

  current_dir[strlen(current_dir) - 1] = '\0';
  return current_dir;
}

void lsh_loop(void) {
  char *line;
  char **args;
  int status;
  char *current_dir;
  struct History hist;
  init_hist(&hist);
  do {
    current_dir = lsh_current_dir();
    printf("%s%s > %s", color, current_dir, DEFAULT);
    dynamic_read_line(&line, 500, &hist, current_dir); // lsh_read_line();
    char *temp_non_div_line = (char *)malloc(sizeof(char) * strlen(line));
    memcpy(temp_non_div_line, line, strlen(line));
    printf("\n");
    args = lsh_split_line(line);
    status = lsh_execute(args);

    if (status) {
      add(&hist, temp_non_div_line);
    }

    free(temp_non_div_line);
    free(line);
    free(args);

    printf("\033[0G\033[K");
  } while (status);
}

int main(int argc, char **argv) {
  turn_off_echo();
  lsh_loop();
  turn_on_echo();
  return EXIT_SUCCESS;
}
