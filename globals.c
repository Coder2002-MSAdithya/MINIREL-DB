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
const int attrCat_numRecs = RELCAT_NUMATTRS + ATTRCAT_NUMATTRS + NT_ATTRCAT;
const int relCat_numPgs = ((NUM_CATS + NT_RELCAT + relcat_recsPerPg - 1) / relcat_recsPerPg);
const int attrCat_numPgs = ((attrCat_numRecs + attrcat_recsPerPg - 1) / attrcat_recsPerPg);

//relcat record for relcat
RelCatRec Relcat_rc = {RELCAT, relcat_recLength, relcat_recsPerPg, RELCAT_NUMATTRS, NUM_CATS + NT_RELCAT, relCat_numPgs};

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

Student students[] = {
    {"Charlie Brown", 0, 3.00},
    {"Lucy", 1, 2.50},
    {"Sally", 2, 2.00},
    {"Peppermint Patty", 3, 1.20},
    {"Linus", 4, 4.00},
    {"Rerun", 5, 3.70},
    {"Schroeder", 6, 4.00},
    {"Snoopy", 7, 3.97},
    {"Pig Pen", 8, 3.35},
    {"Violet", 9, 2.95},
    {"Belle", 10, 3.30},
    {"Woodstock", 11, 3.30},
    {"Spike", 12, 2.90},
    {"Garfield", 13, 3.20},
    {"Odie", 14, 0.30},
    {"John", 15, 3.00},
    {"Nermal", 16, 4.00},
    {"Arlene", 17, 2.67},
    {"Hi", 18, 3.50},
    {"Lois", 19, 3.60},
    {"Kathy", 20, 3.40},
    {"Andrea", 21, 3.40},
    {"Irving", 22, 3.25},
    {"Opus", 23, 2.50},
    {"Milo", 24, 3.80},
    {"Blondi", 25, 2.70},
    {"Dagwood", 26, 2.80},
    {"Kermit", 27, 3.90},
    {"Miss Piggy", 28, 2.70},
    {"Gonzo", 29, 2.20},
    {"Dr. Teeth", 30, 3.40},
    {"Zoot", 31, 1.80},
    {"Janice", 32, 1.50},
    {"Fozzie", 33, 3.00},
    {"Rowlf", 34, 3.80},
    {"Robin", 35, 3.50},
    {"Scooter", 36, 3.95},
    {"Dr. Bunsen Honeydew", 37, 4.00},
    {"Beaker", 38, 3.20},
    {"Animal", 39, 1.00},
    {"Link Hogthrob", 40, 2.00},
    {"The Swedish Chef", 41, 2.80},
    {"Statler", 42, 3.40},
    {"Waldorf", 43, 3.30},
    {"Sam the Eagle", 44, 3.90},
    {"Big Bird", 45, 3.75},
    {"Ernie", 46, 3.00},
    {"Bert", 47, 3.20},
    {"Oscar", 48, 3.40}
};

Professor professors[] = {
        {"Jack Smith", 0, 100.00, "Asst Professor"},
        {"Jill Smith", 1, 100.00, "Emeritus Professor"},
        {"Robert Fountain", 2, 1000.00, "Senior Professor"},
        {"Kenneth Hoffmann", 3, 1000.00, "Asso Professor"},
        {"Jeffrey Ullman", 4, 100.50, "Senior Professor"},
        {"Alfred Aho", 5, 125.00, "Professor"},
        {"Chris Date", 6, 5000.00, "Professor"},
        {"Gio Wiederhold", 7, 100.00, "Professor"},
        {"Shyam Navathe", 8, 800.00, "Associate Professor"},
        {"S Bing Yao", 9, 20000.00, "Emeritus Professor"},
        {"Mario Scholnick", 10, 20000.00, "Professor"}
};

