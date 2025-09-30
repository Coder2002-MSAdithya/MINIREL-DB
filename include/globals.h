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

extern const int relcat_recLength;
extern const int attrcat_recLength;
extern const int relcat_recsPerPg;
extern const int attrcat_recsPerPg;
extern const int attrCat_numRecs;
extern const int relCat_numPgs;
extern const int attrCat_numPgs;

extern RelCatRec Relcat_rc;
extern RelCatRec Relcat_ac;
extern AttrCatRec Attrcat_rrelName;
extern AttrCatRec Attrcat_recLength;
extern AttrCatRec Attrcat_recsPerPg;
extern AttrCatRec AttrCat_numAttrs;
extern AttrCatRec AttrCat_numRecs;
extern AttrCatRec AttrCat_numPgs;
extern AttrCatRec AttrCat_offset;
extern AttrCatRec AttrCat_length;
extern AttrCatRec AttrCat_type;
extern AttrCatRec AttrCat_attrName;
extern AttrCatRec AttrCat_arelName;
#endif

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
