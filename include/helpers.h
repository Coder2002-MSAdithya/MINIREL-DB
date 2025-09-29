#ifndef HELPERS_H
#define HELPERS_H
#include <stdbool.h>
extern bool isValidPath(const char *path);
int remove_all_entry(const char *path);
int remove_contents(const char *dirpath);
#endif