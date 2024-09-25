#include "history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void init_hist(struct History *self) {
  //*self = {0, 0, NULL};
  self->history = NULL;
  self->length = 0;
  self->index = -1;
}

void add(struct History *self, char *command) {
  if (!self) {
    init_hist(self);
  }

  if (command) {
    if (self->history && self->length > 0) {
      // check in the non empty history if the latest command isn't the same as
      // the one to add
      if (strcmp(self->history[self->length - 1], command)) {
        self->history =
            (char **)realloc(self->history, sizeof(char *) * (++self->length));
      }
      // if it's already added we're going to simply ignore it
    } else {
      self->length = 1;
      self->history = (char **)malloc(sizeof(char *));
    }

    // this will be the same process regardless if it's the first item pushed or
    // not
    self->history[self->length - 1] =
        (char *)malloc(sizeof(char) * (strlen(command) + 1));

    strcpy(self->history[self->length - 1], command);
    self->history[self->length - 1][strlen(command)] = '\0';

  } else {
    fprintf(stderr, "Error: void add(struct History *self, char *command)\n\t- "
                    "Input *command cannot be NULL\n");
  }
}

char *upValue(struct History *self) {
  if (self && self->length >= 0) {
    if (self->index == -1) {
      // never used;
      self->index = self->length; // point to the last item + 1, in order to
                                  // return history[--item];
    }

    if (self->index == 0 || self->length == 0) {
      self->index = 0;
      return "\a"; // Ring the bell!
    } else {
      return self->history[--self->index];
    }

  } else {
    fprintf(stderr, "Error: char *upValue(struct History *self)\n\t");
    if (!self) {
      printf("- struct History *self is not initialize\n");
    } else if (!self->history || self->length <= 0) {
      printf("- Not initialize or empty history\n");
    }
    return NULL;
  }
}

char *downValue(struct History *self) {
  if (self && self->length >= 0) {

    if (self->index == -1) {
      self->index = self->length;
    }

    if (self->index == (self->length) || self->length == 0) {
      self->index = -1;
      return "\a";
    } else {
      return self->history[self->index++];
    }
  } else {
    fprintf(stderr, "Error: char *downValue(struct History *self)\n\t");
    if (!self) {
      printf("- struct History *self is not initialize\n");
    } else if (!self->history || self->length <= 0) {
      printf("- Not initialize or empty history\n");
    }
    return NULL;
  }
}
