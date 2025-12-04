/************************INCLUDES*******************************/

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include "../include/error.h"
#include "../include/defs.h"
#include "../include/globals.h"
#include "../include/insertrec.h"


/*------------------------------------------------------------

FUNCTION CreateRel(relName, relNum)

PARAMETER DESCRIPTION:
    relName : Name of the new relation to be created.
    relNum  : Relation number of an already-open source relation whose schema will be copied into the new relation.

FUNCTION DESCRIPTION:
    This routine creates a file named `relName`, and initializes its schema by duplicating the attribute descriptors of the relation identified by `relNum`.
    The procedure performs the following:
        - Validates the new relation name length.
        - Creates an empty file for the relation.
        - Copies the RelCat record of the source relation, except that numPgs and numRecs are reset to 0.
        - Inserts a new entry for the relation in the relation catalog (relcat).
        - For each attribute of the source relation, creates a corresponding AttrCat entry:
            • relName replaced with new name
            • hasIndex reset to 0
            • index metadata cleared
        - The new heap file is empty but ready to accept tuples.
    No data pages are allocated at creation time.

ALGORITHM:
    1) Copy relcat entry from source relation `relNum`.
    2) Reset numPgs = 0 and numRecs = 0.
    3) Validate new relation name length does not exceed RELNAME.
    4) Create an empty file with the same name.
    5) Insert the new RelCatRec into relcat using InsertRec().
    6) For each attribute descriptor of the source relation:
        a) Copy the AttrCatRec.
        b) Replace its relName with the new name.
        c) Reset index metadata (hasIndex, nKeys, nPages).
        d) Insert into attrcat via InsertRec().
    7) Return OK.

GLOBAL VARIABLES MODIFIED:
    db_err_code  : set on error
    relcat/attrcat entries updated via InsertRec()

ERRORS REPORTED:
    REL_LENGTH_EXCEEDED  – new relation name too long
    FILESYSTEM_ERROR     – File creation failed

------------------------------------------------------------*/

/* Create a new relation with name relName with same attribute descriptors as relNum */
int CreateRel(char *relName, int relNum)
{
    RelCatRec new_rc = catcache[relNum].relcat_rec;
    new_rc.numPgs = new_rc.numRecs = 0;

    if(strlen(relName) >= RELNAME)
    {
        db_err_code = REL_LENGTH_EXCEEDED;
        return NOTOK;
    }

    AttrDesc *ptr = catcache[relNum].attrList;
    strncpy(new_rc.relName, relName, RELNAME);

    FILE *fptr = fopen(relName, "w");

    if(!fptr)
    {
        db_err_code = FILESYSTEM_ERROR;
        return NOTOK;
    }

    InsertRec(RELCAT_CACHE, &new_rc);

    for(;ptr;ptr=ptr->next)
    {
        AttrCatRec ac = ptr->attr;
        strncpy(ac.relName, relName, RELNAME);
        ac.hasIndex = 0;
        ac.nKeys = ac.nPages = 0;
        InsertRec(ATTRCAT_CACHE, &ac);
    }

    return OK;
}
