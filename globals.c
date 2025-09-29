#include "include/globals.h"
#include "include/defs.h"

CacheEntry catcache[MAXOPEN];	/* catalog cache */
Buffer buffer[MAXOPEN];         /* buffer pool */
char ORIG_DIR[MAX_PATH_LENGTH]; /*original invoked directory */
char DB_DIR[MAX_PATH_LENGTH];  /* database working directory */
