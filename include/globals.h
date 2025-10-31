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
extern bool debug_flag;
extern int db_err_code;

extern const int relcat_recLength;
extern const int attrcat_recLength;
extern const int relcat_recsPerPg;
extern const int attrcat_recsPerPg;
extern const int relcat_numRecs;
extern const int attrcat_numRecs;
extern const int relcat_numPgs;
extern const int attrcat_numPgs;

extern RelCatRec RelCat_rc;
extern RelCatRec RelCat_ac;
extern AttrCatRec AttrCat_rrelName;
extern AttrCatRec AttrCat_recLength;
extern AttrCatRec AttrCat_recsPerPg;
extern AttrCatRec AttrCat_numAttrs;
extern AttrCatRec AttrCat_numRecs;
extern AttrCatRec AttrCat_numPgs;
extern AttrCatRec AttrCat_offset;
extern AttrCatRec AttrCat_length;
extern AttrCatRec AttrCat_type;
extern AttrCatRec AttrCat_attrName;
extern AttrCatRec AttrCat_arelName;
// extern RelCatRec adv_rc, nn_rc, prof_rc, stud_rc;
// extern AttrCatRec profs_name, profs_id, profs_des, profs_sal;
// extern AttrCatRec stud_id, stud_name, stud_stp;

// extern Student students[49];
// extern Professor professors[11];
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


int FlushPage(int relNum);
int FindRelNum(const char *relName);
