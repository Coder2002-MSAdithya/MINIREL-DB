#ifndef _FREEMAP_H
#define _FREEMAP_H
void build_fmap_filename(const char *relName, char *fname, size_t buflen);
int FreeMapExists(const char *relName);
int CreateFreeMap(const char *relName);
int AddToFreeMap(const char *relName, short pageNum);
int DeleteFromFreeMap(const char *relName, short pageNum);
int FindFreeSlot(const char *relName);
#endif