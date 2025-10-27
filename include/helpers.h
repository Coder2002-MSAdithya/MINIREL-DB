#ifndef HELPERS_H
#define HELPERS_H
#include <stdlib.h>
#include <stdbool.h>
extern bool isValidPath(const char *path);
int remove_all_entry(const char *path);
int remove_contents(const char *dirpath);
int ceil_div(int a, int b);
void print_page_hex(const char *buf);
int writeRecsToFile(const char *, void *, int, int, char);
int float_cmp(double a, double b, double rel_eps, double abs_eps);
double dmax(double, double);
Rid IncRid(Rid rid, int recsPerPg);
int FreeLinkedList(void **headPtr, size_t nextOff);
bool isValidRid(Rid rid);
#endif