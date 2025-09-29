/****************************************************************
		GLOBAL VARIABLE AND FUNCTION PROTOTYPES DECLARATIONS
****************************************************************/

#ifndef GLOBALS_MINIREL
#define GLOBALS_MINIREL
#include "defs.h"
#include <stdbool.h>
extern CacheEntry catcache[MAXOPEN];	/* catalog cache */
extern Buffer buffer[MAXOPEN];	/* buffer pool */
extern char ORIG_DIR[MAX_PATH_LENGTH];
extern char DB_DIR[MAX_PATH_LENGTH];
extern bool db_open;
extern bool print_flag;
#endif

int OpenDB (int argc, char **argv);
int CloseDB (int argc, char **argv);
int DestroyDB (int argc, char **argv);
int CreateCats();
int Create (int argc, char **argv);
int Destroy (int argc, char **argv);
int Load (int argc, char **argv);
int Print (int argc, char **argv);
int BuildIndex (int argc, char **argv);
int DropIndex (int argc, char **argv);
int Quit (int argc, char **argv);
int Insert (int argc, char **argv);
int Delete (int argc, char **argv);
int Project (int argc, char **argv);
int Select (int argc, char **argv);
int Join (int argc, char **argv);
