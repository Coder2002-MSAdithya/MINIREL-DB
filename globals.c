#include "include/globals.h"
#include "include/helpers.h"
#include "include/defs.h"
#include <stddef.h>
#include <stdbool.h>

CacheEntry catcache[MAXOPEN];	/* catalog cache */
Buffer buffer[MAXOPEN];         /* buffer pool */
char ORIG_DIR[MAX_PATH_LENGTH]; /*original invoked directory */
char DB_DIR[MAX_PATH_LENGTH];  /* database working directory */
bool db_open = false;
bool print_flag = true;

const int relcat_recLength = (int)sizeof(RelCatRec);
const int attrcat_recLength = (int)sizeof(AttrCatRec);
const int relcat_recsPerPg = (PAGESIZE - HEADER_SIZE) / relcat_recLength;
const int attrcat_recsPerPg = (PAGESIZE - HEADER_SIZE) / attrcat_recLength;
const int attrCat_numRecs = RELCAT_NUMATTRS + ATTRCAT_NUMATTRS;
const int relCat_numPgs = ((NUM_CATS + relcat_recsPerPg - 1) / relcat_recsPerPg);
const int attrCat_numPgs = ((attrCat_numRecs + attrcat_recsPerPg - 1) / attrcat_recsPerPg);

//relcat record for relcat
RelCatRec Relcat_rc = {RELCAT, relcat_recLength, relcat_recsPerPg, RELCAT_NUMATTRS, NUM_CATS, relCat_numPgs};

//relcat record for attrcat
RelCatRec Relcat_ac = {ATTRCAT, attrcat_recLength, attrcat_recsPerPg, ATTRCAT_NUMATTRS, attrCat_numRecs, attrCat_numPgs};

//attrcat record for relName column of relcat
AttrCatRec Attrcat_rrelName = {offsetof(RelCatRec, relName), RELNAME, 's', "relName", RELCAT};

//attrcat record for recLength column of relcat
AttrCatRec Attrcat_recLength = {offsetof(RelCatRec, recLength), sizeof(int), 'i', "recLength", RELCAT};

//attrcat record for recsPerPg column of relcat
AttrCatRec Attrcat_recsPerPg = {offsetof(RelCatRec, recsPerPg), sizeof(int), 'i', "recsPerPg", RELCAT};

//attrcat record for numAttrs column of relcat
AttrCatRec AttrCat_numAttrs = {offsetof(RelCatRec, numAttrs), sizeof(int), 'i', "numAttrs", RELCAT};

//attrcat record for numRecs column of relcat
AttrCatRec AttrCat_numRecs = {offsetof(RelCatRec, numRecs), sizeof(int), 'i', "numRecs", RELCAT};

//attrcat record for numPgs column of relcat
AttrCatRec AttrCat_numPgs = {offsetof(RelCatRec, numPgs), sizeof(int), 'i', "numPgs", RELCAT};

//attrcat record for offset column of attrcat
AttrCatRec AttrCat_offset = {offsetof(AttrCatRec, offset), sizeof(int), 'i', "offset", ATTRCAT};

//attrcat record for length column of attrcat
AttrCatRec AttrCat_length = {offsetof(AttrCatRec, length), sizeof(int), 'i', "length", ATTRCAT};

//attrcat record for type column of attrcat
AttrCatRec AttrCat_type = {offsetof(AttrCatRec, type), sizeof(char), 's', "type", ATTRCAT};

//attrcat record for attrName column of attrCat
AttrCatRec AttrCat_attrName = {offsetof(AttrCatRec, attrName), ATTRNAME, 's', "attrName", ATTRCAT};

//attrcat record for relName column of attrCat
AttrCatRec AttrCat_arelName = {offsetof(AttrCatRec, relName), RELNAME, 's', "relName", ATTRCAT};