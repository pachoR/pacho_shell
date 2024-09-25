#include <ctype.h>

#include "history.c"
#include "history.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/_types/_ssize_t.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

struct line_stack {
  char *line_stack;
  int len;
};

void delete_line_stack(struct line_stack *line, int *idx) {
  /*
   * The only reason for sending idx as an pointer it's to manage the cursor
   * index decreasing inside the function instead to increasing it each time we
   * call the function in order to avoid a confusion
   */
  if (*idx > 0) {
    *idx -= 1;
  }
  int index = *idx;

  if (!line || index < 0 || index > line->len) {
    return;
  }

  if (index == 0 && line->len == 0) {
    return;

  } else {
    int f_len = index;
    int s_len = strlen(line->line_stack) - index;
    // s_len dosen't substract the char to delete to consider the \0 char

    char *first_str = (char *)malloc(sizeof(char) * f_len);
    char *second_str = (char *)malloc(sizeof(char) * s_len);

    memcpy(first_str, line->line_stack, f_len);
    memcpy(second_str, &line->line_stack[index + 1], s_len);

    free(line->line_stack);
    line->line_stack = (char *)malloc(f_len + s_len);
    memcpy(line->line_stack, first_str, f_len);
    memcpy(&line->line_stack[f_len], second_str, s_len);
    line->len = strlen(line->line_stack);
  }
}

void write_line_stack(struct line_stack *line, char c, int *idx) {
  /*
   * The only reason for sending idx as an pointer it's to manage the cursor
   * index increasing inside the function instead to increasing it each time we
   * call the function in order to avoid a confusion
   */

  int index = *idx;

  if (!line || index < 0 || index > line->len) {
    return;
  }

  if (index == 0 && line->len == 0) {
    line->len++;
    line->line_stack = (char *)malloc(sizeof(char) * line->len);
    line->line_stack[index] = c;
    *idx += 1;

  } else {
    int f_len = index;
    int s_len = strlen(line->line_stack) - index + 1; // for \0 char

    char *first_str = (char *)malloc(sizeof(char) * f_len);
    char *second_str = (char *)malloc(sizeof(char) * s_len);

    memcpy(first_str, line->line_stack, f_len);
    memcpy(second_str, &line->line_stack[index], s_len);

    char *final_string = (char *)malloc(sizeof(char) * (f_len + s_len + 1));
    second_str[s_len] = '\0';

    memcpy(final_string, first_str, f_len);
    final_string[index] = c;
    memcpy(&final_string[index + 1], second_str, s_len);

    free(line->line_stack);
    line->line_stack = final_string;
    line->line_stack[f_len + s_len] = '\0';
    line->len = strlen(line->line_stack);
    *idx += 1;

    free(first_str);
    free(second_str);
  }
}

void turn_off_echo() {
  struct termios term;
  tcgetattr(STDIN_FILENO, &term);
  term.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void turn_on_echo() {
  struct termios term;
  tcgetattr(STDIN_FILENO, &term);
  term.c_lflag |= (ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

int top = 0;

void dynamic_read_line(char **str, int bufsize, struct History *hist,
                       char *current_dir) {

  int dir_offset = strlen(current_dir) + 4;
  int cursor_idx = 0;
  struct line_stack *line =
      (struct line_stack *)malloc(sizeof(struct line_stack));
  if (line == NULL) {
    fprintf(stderr, "struct line_stack allocation error");
    return; // failure
  }
  char c;

  line->line_stack = NULL;
  line->len = 0;
  printf("\033[%d", dir_offset);
  do {
    c = getchar();

    if (c == '\n') {
      break;
    }

    if (c == 127) {
      delete_line_stack(line, &cursor_idx);

      printf("\033[%dG", dir_offset);
      for (int i = 0; i <= line->len; i++) {
        printf(" ");
        // this for clearing the screen of the past data, because we will see
        // the old data
      }

      printf("\033[%dG", dir_offset);
      if (line->line_stack) {
        printf("%s", line->line_stack);
      }

      if (cursor_idx == 0) {
        printf("\033[%dG", dir_offset);
      } else {
        printf("\033[%dG", cursor_idx + dir_offset);
      }
    } else if (c == '\033') {
      c = getchar();
      if (c == '[') {
        c = getchar();
        switch (c) {
        case 'A': {

          if (line->line_stack) {
            printf("\033[%dG\033[K", dir_offset);
            free(line->line_stack);
          }

          char *up_value = upValue(hist);
          line->len = strlen(up_value);
          line->line_stack = (char *)malloc(sizeof(char) * strlen(up_value));
          memcpy(line->line_stack, up_value, strlen(up_value));
          printf("\033[%dG", dir_offset);
          printf("%s", line->line_stack);
          fflush(stdout);
          break;
        }
        case 'B': {

          if (line->line_stack) {
            printf("\033[%dG\033[K", dir_offset);
            free(line->line_stack);
          }

          char *down_value = downValue(hist);
          line->len = strlen(down_value);
          line->line_stack = (char *)malloc(sizeof(char) * strlen(down_value));
          memcpy(line->line_stack, down_value, strlen(down_value));
          printf("\033[%dG", dir_offset);
          printf("%s", line->line_stack);
          fflush(stdout);

          break;
        }

        case 'C': {
          if (cursor_idx < line->len) {
            printf("\033[C");
            cursor_idx++;
          }
          break;
        }

        case 'D': {

          if (cursor_idx > 0) {
            printf("\033[D");
            cursor_idx--;
          }
          break;
        }
        } // end switch
      }

    } else {
      write_line_stack(line, c, &cursor_idx);

      // printf("\033[0G%s > ", current_dir);
      printf("\033[%dG", dir_offset);

      printf("%s", line->line_stack);
      if (cursor_idx == 0) {
        printf("\033[%dG", dir_offset);
      } else {
        printf("\033[%dG", cursor_idx + dir_offset);
      }

      fflush(stdout);
    }
  } while (cursor_idx < bufsize && c != '\n');

  *str = (char *)malloc(sizeof(char) * line->len);
  line->line_stack[line->len] = '\n';
  memcpy(*str, line->line_stack, line->len);
}

/*
 *   This file contains the implementation of a dynamic line reader, working
 * each character at the time. In order to achieved that behavior it's necessary
 * to customize the termios by turning off ECHO and ICANON flag.
 *
 *   We use the first one to avoid printing certain characters that will trigger
 * the History functions (up & down key) and the restricted cursor navigation
 * (left & right), so by turning off echoing we'll be responsible for printing
 * out the chars inputted, to be seen by the user.
 *
 *   The second flag will be necesarry to treat the input manage to work char by
 * char, instead of the default line by line. This will let us to print the
 * chars in a dynamic way by using fflush(), if ICANON on the chars could only
 * be printed until the user end the line by using enter key.
 */
