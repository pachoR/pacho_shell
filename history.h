#ifndef HISTORY_H
#define HISTORY_H

struct History {
  int index, length;
  char **history;
  void (*add)(struct History *self, char *command);
  // char *(*upValue)(struct History *self);
  // char *(*downValue)(struct History *self);
  // void (*init_hist)(struct History *self);
};

/*
void add(struct History *self, char *command);
char *upValue(struct History *self);
char *downValue(struct History *self);
void init(struct History *self);
*/
#endif
