#ifndef HELPERS_H
#define HELPERS_H
#include <stdbool.h>
extern bool isValidPath(const char *path);
int remove_all_entry(const char *path);
int remove_contents(const char *dirpath);
int ceil_div(int a, int b);
void print_page_hex(const char *buf);
int writeRecsToFile(const char *, void *, int, int, char);
#endif