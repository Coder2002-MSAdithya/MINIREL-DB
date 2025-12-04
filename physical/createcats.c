/************************INCLUDES*******************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"


/*------------------------------------------------------------

FUNCTION CreateRelCat()

FUNCTION DESCRIPTION:
    This routine creates the relation catalog (relcat) by writing the fixed metadata records describing the system catalogs themselves. 
    The relcat contains one RelCatRec for every relation known to the system at DB-creation time.
    For MINIREL, there are exactly two catalog relations:
    1) relcat   — stores relation-level metadata
    2) attrcat  — stores attribute-level metadata
    This function prepares an in-memory array containing both RelCatRec objects (Relcat_rc and Relcat_ac) and writes them into the relcat file using writeRecsToFile().

ALGORITHM:
    1) Construct an array relcat_recs[] containing the RelCatRec entries for relcat and attrcat.
    2) Call writeRecsToFile() with:
            - filename = RELCAT
            - array of records
            - number of records
            - record size
            - page fill character '$'
    3) Return the status from writeRecsToFile().

RETURNS:
    OK      – catalog successfully created.
    NOTOK   – failure reported from writeRecsToFile().

ERRORS REPORTED:
    None directly.

GLOBAL VARIABLES MODIFIED:
    None directly.

------------------------------------------------------------*/

int CreateRelCat()
{
    // Create array of RelCatRecs to insert them one by one into page
    RelCatRec relcat_recs[] = {Relcat_rc, Relcat_ac};

    // Write the records in an array to the catalog file in pages
    return writeRecsToFile(RELCAT, relcat_recs, NUM_CATS, sizeof(RelCatRec), '$');
}


/*------------------------------------------------------------

FUNCTION CreateAttrCat()

SPECIFICATIONS:
    This routine creates the attribute catalog (attrcat), which stores metadata for every attribute of every relation known to the system at DB-creation time.
    The function assembles all AttrCatRec objects corresponding to relcat’s attributes and attrcat’s attributes, and writes them sequentially into the attrcat file.

ALGORITHM:
    1) Construct attrcat_recs[], containing one entry per attribute of the catalog relations.
    2) Call writeRecsToFile() with:
            - filename = ATTRCAT
            - the array of AttrCatRec
            - record count = attrCat_numRecs
            - record size
            - page fill character '!'
    3) Return the result of writeRecsToFile().

RETURNS:
    OK      – attribute catalog successfully created.
    NOTOK   – failure reported from writeRecsToFile().

ERRORS REPORTED:
    None directly.

GLOBAL VARIABLES MODIFIED:
    None directly.

IMPLEMENTATION NOTES:
    - The attribute catalog stores offsets, lengths, and types for every attribute in relcat and attrcat.
    - '!' is used as the page fill character to distinguish attrcat pages from relcat ('$').

------------------------------------------------------------*/

int CreateAttrCat()
{
    // Create array of AttrCatRecs to insert them one by one into page
    AttrCatRec attrcat_recs[] = {
        Attrcat_rrelName,
        Attrcat_recLength,
        Attrcat_recsPerPg,
        AttrCat_numAttrs,
        AttrCat_numRecs,
        AttrCat_numPgs,
        AttrCat_offset,
        AttrCat_length,
        AttrCat_type,
        AttrCat_attrName,
        AttrCat_arelName,
        AttrCat_hasIndex,
        AttrCat_nPages,
        AttrCat_nKeys,
    };

    // Write the records in an array to the catalog file in pages
    return writeRecsToFile(ATTRCAT, attrcat_recs, attrCat_numRecs, sizeof(AttrCatRec), '!');
}


/*------------------------------------------------------------

FUNCTION CreateCats()

SPECIFICATIONS:
    Creates both system catalogs: relcat and attrcat.
    This routine is invoked during database creation to initialize the metadata layer of MINIREL. 
    It calls CreateRelCat() and CreateAttrCat(), validating the result of each operation.

ALGORITHM:
    1) Call CreateRelCat().
    2) Call CreateAttrCat().
    3) If both succeed → return OK.
    4) Otherwise set db_err_code = CAT_CREATE_ERROR and return NOTOK.

RETURNS:
    OK      – both catalogs successfully created.
    NOTOK   – at least one catalog creation failed.

ERRORS REPORTED:
    CAT_CREATE_ERROR

GLOBAL VARIABLES MODIFIED:
    db_err_code (only on failure).

------------------------------------------------------------*/

int CreateCats()
{
    if(CreateRelCat() == OK && CreateAttrCat() == OK)
    {
        return OK;
    }

    db_err_code = CAT_CREATE_ERROR;
    return NOTOK;
}