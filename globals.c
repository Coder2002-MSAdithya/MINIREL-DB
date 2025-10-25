#include "include/globals.h"
#include "include/helpers.h"
#include "include/defs.h"
#include <stddef.h>
#include <stdbool.h>

CacheEntry catcache[MAXOPEN];	/* catalog cache */
Buffer buffer[MAXOPEN];         /* buffer pool */
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

Student students[] = {
        {"Charlie Brown", 1, 3.00},
        {"Lucy", 2, 2.50},
        {"Sally", 3, 2.00},
        {"Peppermint Patty", 4, 1.20},
        {"Linus", 5, 4.00},
        {"Rerun", 6, 3.70},
        {"Schroeder", 7, 4.00},
        {"Snoopy", 8, 3.97},
        {"Pig Pen", 9, 3.35},
        {"Violet", 10, 2.95},
        {"Belle", 11, 3.30},
        {"Woodstock", 12, 3.30},
        {"Spike", 13, 2.90},
        {"Garfield", 14, 3.20},
        {"Odie", 15, 0.30},
        {"John", 16, 3.00},
        {"Nermal", 17, 4.00},
        {"Arlene", 18, 2.67},
        {"Hi", 19, 3.50},
        {"Lois", 20, 3.60},
        {"Kathy", 21, 3.40},
        {"Andrea", 22, 3.40},
        {"Irving", 23, 3.25},
        {"Opus", 24, 2.50},
        {"Milo", 25, 3.80},
        {"Blondi", 26, 2.70},
        {"Dagwood", 27, 2.80},
        {"Kermit", 28, 3.90},
        {"Miss Piggy", 29, 2.70},
        {"Gonzo", 30, 2.20},
        {"Dr. Teeth", 31, 3.40},
        {"Zoot", 32, 1.80},
        {"Janice", 33, 1.50},
        {"Fozzie", 34, 3.00},
        {"Rowlf", 35, 3.80},
        {"Robin", 36, 3.50},
        {"Scooter", 37, 3.95},
        {"Dr. Bunsen Honeydew", 38, 4.00},
        {"Beaker", 39, 3.20},
        {"Animal", 40, 1.00},
        {"Link Hogthrob", 41, 2.00},
        {"The Swedish Chef", 42, 2.80},
        {"Statler", 43, 3.40},
        {"Waldorf", 44, 3.30},
        {"Sam the Eagle", 45, 3.90},
        {"Big Bird", 46, 3.75},
        {"Ernie", 47, 3.00},
        {"Bert", 48, 3.20},
        {"Oscar", 49, 3.40}
    };

Professor professors[] = {
        {"Jack Smith", 1, 100.00, "Asst Professor"},
        {"Jill Smith", 2, 100.00, "Emeritus Professor"},
        {"Robert Fountain", 3, 1000.00, "Senior Professor"},
        {"Kenneth Hoffmann", 4, 1000.00, "Asso Professor"},
        {"Jeffrey Ullman", 5, 100.50, "Senior Professor"},
        {"Alfred Aho", 6, 125.00, "Professor"},
        {"Chris Date", 7, 5000.00, "Professor"},
        {"Gio Wiederhold", 8, 100.00, "Professor"},
        {"Shyam Navathe", 9, 800.00, "Associate Professor"},
        {"S Bing Yao", 10, 20000.00, "Emeritus Professor"},
        {"Mario Scholnick", 11, 20000.00, "Professor"}
};

