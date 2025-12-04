/************************INCLUDES*******************************/

#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/openrel.h"
#include "../include/findrelattr.h"
#include "../include/writerec.h"
#include "../include/unpinrel.h"
#include <stdio.h>
#include <stddef.h>
#include <string.h>


/*------------------------------------------------------------

FUNCTION BuildIndex (argc, argv)

PARAMETER DESCRIPTION:
    argc → number of command arguments.
    argv → argument vector.

SPECIFICATIONS:
    argv[0] = "buildindex"
    argv[1] = relation name
    argv[2] = attribute name
    argv[argc] = NIL

FUNCTION DESCRIPTION:
    Creates an index on a specified attribute of a given relation. 
    The routine enforces strict MINIREL rules: indices may not be created on the system catalogs, relations must be empty before indexing, and an index cannot be rebuilt if it already exists. 
    The routine accesses metadata via relcat and attrcat, locates the desired attribute descriptor, and updates its hasIndex flag. 
    No physical index structure is constructed, only catalog metadata is changed.

ALGORITHM:
    1) Verify that a database is open.
    2) Extract relation name and attribute name from argv.
    3) Open the relation using OpenRel(); if it does not exist, report RELNOEXIST.
    4) Reject attempts to index catalog relations (relcat, attrcat).
    5) Find the attribute using FindRelAttr(); if not found, report ATTRNOEXIST.
    6) Ensure the relation is empty:
        - relcat_rec.numRecs must be 0
        - relcat_rec.numPgs must be 0
    7) Check whether the attribute already has an index; if so, report IDXEXIST.
    8) Set hasIndex = 1 in the AttrCatRec and write the updated record back to attrcat via WriteRec().
    9) Print a success message and return OK.

BUGS:
    • Assumes the empty-relation restriction is required, the system cannot build indices on existing data.
    • Does not validate numeric vs. string attribute types, any attribute may be indexed.

ERRORS REPORTED:
    DBNOTOPEN
    RELNOEXIST
    METADATA_SECURITY
    ATTRNOEXIST
    INDEX_NONEMPTY
    IDXEXIST
    FILESYSTEM_ERROR (via WriteRec)

GLOBAL VARIABLES MODIFIED:
    • db_err_code
        catcache[].attr.hasIndex for the target attribute

IMPLEMENTATION NOTES:
    • Only updates the catalog metadata; no on-disk index file or structure is created.

------------------------------------------------------------*/

int BuildIndex(int argc, char **argv)
{
    if(!db_open)
    {
        db_err_code = DBNOTOPEN;
        return ErrorMsgs(db_err_code, print_flag);
    }

    char *relName = argv[1];
    char *attrName = argv[2];

    int r = OpenRel(relName);

    if(r == NOTOK)
    {
        return ErrorMsgs(db_err_code, print_flag);
    }

    if(strncmp(relName, RELCAT, RELNAME) == OK || 
    strncmp(relName, ATTRCAT, RELNAME) == OK)
    {
        printf("CANNOT create or drop indexes on catalog relation %s...\n", relName);
        db_err_code = METADATA_SECURITY;
        return ErrorMsgs(db_err_code, print_flag);
    }

    AttrDesc *attrDesc = FindRelAttr(r, attrName);

    if(!attrDesc)
    {
        printf("Attribute '%s' does NOT exist in relation '%s' of the DB.\n", relName, attrName);
        printCloseStrings(ATTRCAT_CACHE, offsetof(AttrCatRec, attrName), attrName, relName);
        db_err_code = ATTRNOEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    int numPgs = catcache[r].relcat_rec.numRecs;
    int numRecs = catcache[r].relcat_rec.numPgs;

    if(numPgs || numRecs)
    {
        db_err_code = INDEX_NONEMPTY;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if(attrDesc->attr.hasIndex)
    {
        db_err_code = IDXEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    attrDesc->attr.hasIndex = 1;
    if(WriteRec(ATTRCAT_CACHE, &(attrDesc->attr), attrDesc->attrCatRid) == NOTOK)
    {
        return ErrorMsgs(db_err_code, print_flag);
    }

    UnPinRel(r);
    printf("Built index successfully on attribute %s of relation %s\n", 
    attrName, relName);

    return OK;
}