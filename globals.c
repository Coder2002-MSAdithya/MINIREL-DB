#include "include/globals.h"
#include "include/helpers.h"
#include "include/defs.h"
#include <stddef.h>
#include <stdbool.h>

CacheEntry catcache[MAXOPEN];	/* catalog cache */
Buffer buffer[MAXOPEN];         /* buffer pool */
IdxBuf idx_buffer[MAXOPEN];     /* buffer pool for the indexes */
char ORIG_DIR[MAX_PATH_LENGTH]; /*original invoked directory */
char DB_DIR[MAX_PATH_LENGTH];  /* database working directory */
bool db_open = false;   /* database open */
bool print_flag = true; /* flag to print error messages*/
bool debug_flag = true; /* for debugging purposes */
int db_err_code = OK; /* Global state for last error */

const int relcat_recLength = (int)sizeof(RelCatRec);
const int attrcat_recLength = (int)sizeof(AttrCatRec);
const int relcat_recsPerPg = MIN((PAGESIZE - HEADER_SIZE) / relcat_recLength, SLOTMAP<<3);
const int attrcat_recsPerPg = MIN((PAGESIZE - HEADER_SIZE) / attrcat_recLength, SLOTMAP<<3);
const int attrCat_numRecs = RELCAT_NUMATTRS + ATTRCAT_NUMATTRS;
const int relCat_numPgs = ((NUM_CATS + NT_RELCAT + relcat_recsPerPg - 1) / relcat_recsPerPg);
const int attrCat_numPgs = ((attrCat_numRecs + attrcat_recsPerPg - 1) / attrcat_recsPerPg);

//relcat record for relcat
RelCatRec Relcat_rc = {RELCAT, relcat_recLength, relcat_recsPerPg, RELCAT_NUMATTRS, NUM_CATS, relCat_numPgs};

//relcat record for attrcat
RelCatRec Relcat_ac = {ATTRCAT, attrcat_recLength, attrcat_recsPerPg, ATTRCAT_NUMATTRS, attrCat_numRecs, attrCat_numPgs};

//attrcat record for relName column of relcat
AttrCatRec Attrcat_rrelName = {offsetof(RelCatRec, relName), RELNAME, "s", "relName", RELCAT, 0, 0, 0};

//attrcat record for recLength column of relcat
AttrCatRec Attrcat_recLength = {offsetof(RelCatRec, recLength), sizeof(int), "i", "recLength", RELCAT, 0, 0, 0};

//attrcat record for recsPerPg column of relcat
AttrCatRec Attrcat_recsPerPg = {offsetof(RelCatRec, recsPerPg), sizeof(int), "i", "recsPerPg", RELCAT, 0, 0, 0};

//attrcat record for numAttrs column of relcat
AttrCatRec AttrCat_numAttrs = {offsetof(RelCatRec, numAttrs), sizeof(int), "i", "numAttrs", RELCAT, 0, 0, 0};

//attrcat record for numRecs column of relcat
AttrCatRec AttrCat_numRecs = {offsetof(RelCatRec, numRecs), sizeof(int), "i", "numRecs", RELCAT, 0, 0, 0};

//attrcat record for numPgs column of relcat
AttrCatRec AttrCat_numPgs = {offsetof(RelCatRec, numPgs), sizeof(int), "i", "numPgs", RELCAT, 0, 0, 0};

//attrcat record for offset column of attrcat
AttrCatRec AttrCat_offset = {offsetof(AttrCatRec, offset), sizeof(int), "i", "offset", ATTRCAT, 0, 0, 0};

//attrcat record for length column of attrcat
AttrCatRec AttrCat_length = {offsetof(AttrCatRec, length), sizeof(int), "i", "length", ATTRCAT, 0, 0, 0};

//attrcat record for type column of attrcat
AttrCatRec AttrCat_type = {offsetof(AttrCatRec, type), TYPE_COL_SIZE, "s", "type", ATTRCAT, 0, 0, 0};

//attrcat record for attrName column of attrCat
AttrCatRec AttrCat_attrName = {offsetof(AttrCatRec, attrName), ATTRNAME, "s", "attrName", ATTRCAT, 0, 0, 0};

//attrcat record for relName column of attrCat
AttrCatRec AttrCat_arelName = {offsetof(AttrCatRec, relName), RELNAME, "s", "relName", ATTRCAT, 0, 0, 0};

//attrcat record for hasIndex column of attrCat
AttrCatRec AttrCat_hasIndex = {offsetof(AttrCatRec, hasIndex), sizeof(int), "i", "hasIndex", ATTRCAT, 0, 0, 0};

//attrcat record for nPages column of attrCat
AttrCatRec AttrCat_nPages = {offsetof(AttrCatRec, nPages), sizeof(int), "i", "nPages", ATTRCAT, 0, 0, 0};

//attrcat record for nKeys column of attrCat
AttrCatRec AttrCat_nKeys = {offsetof(AttrCatRec, nKeys), sizeof(int), "i", "nKeys", ATTRCAT, 0, 0, 0};