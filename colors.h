#ifndef COLORS_H
#define COLORS_H
#define DEFAULT "\033[0m"
#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define YELLOW "\033[0;33m"
#define BLUE "\033[0;34m"
#define PURPLE "\033[0;35m"
#define CYAN "\033[0;36m"
#define WHITE "\033[0;37m"
const char *COLOR_VALUE[] = {RED, GREEN, YELLOW, BLUE, PURPLE, CYAN, WHITE};
const char *COLOR_LIST[] = {"RED",    "GREEN", "YELLOW", "BLUE",
                            "PURPLE", "CYAN",  "WHITE"};
const int N_COLORS = 7;
#endif
