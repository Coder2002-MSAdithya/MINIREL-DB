/************************INCLUDES*******************************/

#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/openrel.h"
#include "../include/findrelattr.h"
#include "../include/writerec.h"
#include <stdio.h>
#include <stddef.h>
#include <string.h>


/*------------------------------------------------------------

FUNCTION removeIndex (attrPtr)

PARAMETER DESCRIPTION:
    attrPtr → pointer to an AttrDesc structure belonging to an attribute of an open relation. 

FUNCTION DESCRIPTION:
    Clears the index flag for a specific attribute and writes the updated attribute metadata back to the ATTRCAT relation.
    Operational steps:
        - Set hasIndex = 0 within attrPtr->attr.
        - Persist the modified attribute catalog record using WriteRec().

ALGORITHM:
    1) Set attrPtr->attr.hasIndex = 0.
    2) Call WriteRec(ATTRCAT_CACHE, &attrPtr->attr, attrPtr->attrCatRid).
    3) If WriteRec() fails → return NOTOK, otherwise return OK.


ERRORS REPORTED:
    None.

GLOBAL VARIABLES MODIFIED:
    attrPtr->attr (in-memory metadata)
    The corresponding record in ATTRCAT on disk (via WriteRec)

IMPLEMENTATION NOTES:
    - This function must be invoked only after ensuring that the index exists (hasIndex == 1).
    - Caller is responsible for error propagation and user-visible messages.
    - Does not close or flush any relation; purely updates catalog data.

------------------------------------------------------------*/

int removeIndex(AttrDesc *attrPtr)
{
    attrPtr->attr.hasIndex = 0;

    if(WriteRec(ATTRCAT_CACHE, &(attrPtr->attr), attrPtr->attrCatRid) != OK)
    {
        return NOTOK;
    }

    return OK;
}


/*------------------------------------------------------------

FUNCTION DropIndex (argc, argv)

PARAMETER DESCRIPTION:
    argc → number of command arguments
    argv → argument vector

SPECIFICATIONS:
    argv[0] = "dropindex"
    argv[1] = relation name
    argv[2] = (optional) attribute name
    argv[argc] = NIL

FUNCTION DESCRIPTION:
    Removes index metadata from a specified attribute of a relation, or from all attributes if no attribute name is supplied. 
    This routine only modifies catalog data (attrcat). 
    Indexes on catalog relations (relcat, attrcat) are explicitly disallowed.

ALGORITHM:
    1) Check that a database is open.
    2) Extract relation name and attribute name.
    3) Open the relation using OpenRel():
        - If it does not exist, report RELNOEXIST.
    4) Reject attempts to drop indexes on relcat or attrcat (metadata protection).
    5) If an attribute name is provided:
        a) Lookup the attribute in the relation using FindRelAttr().
        b) If not found → ATTRNOEXIST.
        c) If hasIndex == 0 → IDXNOEXIST.
        d) Call removeIndex(), to clear the hasIndex flag and updates attrcat via WriteRec().
        e) Print confirmation.
    6) If no attribute name is provided:
            Iterate through all AttrDesc nodes in the relation’s attribute list; for an attribute with hasIndex == 1, call removeIndex().
            Print confirmation.
    7) Return OK.

ERRORS REPORTED:
    DBNOTOPEN
    RELNOEXIST
    METADATA_SECURITY
    ATTRNOEXIST
    IDXNOEXIST
    FILESYSTEM_ERROR (via WriteRec)

GLOBAL VARIABLES MODIFIED:
    • db_err_code
    • attrcat entries corresponding to the affected attributes
    • catcache[r].attrList[].attr.hasIndex fields

IMPLEMENTATION NOTES (IF ANY):
    • removeIndex() is called to sets hasIndex = 0 and write the updated AttrCatRec back to attrcat.
    • No physical index structures are created or destroyed, MINIREL handles only the catalog metadata at this stage.

------------------------------------------------------------*/

int DropIndex(int argc, char **argv)
{
    if(!db_open)
    {
        db_err_code = DBNOTOPEN;
        return ErrorMsgs(db_err_code, print_flag);
    }

    char *relName = argv[1];
    char *attrName = argc == 3 ? argv[2] : NULL;

    AttrDesc *attrPtr = NULL;

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

    if(argc == 3)
    {
        attrPtr = FindRelAttr(r, attrName);

        if(!attrPtr)
        {
            printf("Attribute '%s' does NOT exist in relation '%s' of the DB.\n", relName, attrName);
            printCloseStrings(ATTRCAT_CACHE, offsetof(AttrCatRec, attrName), attrName, relName);
            db_err_code = ATTRNOEXIST;
            return ErrorMsgs(db_err_code, print_flag);
        }

        if(!attrPtr->attr.hasIndex)
        {
            printf("Index does NOT exist on attribute '%s' of relation '%s'.\n", attrName, relName);
            db_err_code = IDXNOEXIST;
            return ErrorMsgs(db_err_code, print_flag);
        }
    }

    if(attrPtr)
    {
        removeIndex(attrPtr);
        printf("Destroyed index successfully on attribute %s of relation %s\n", 
        attrName, relName);
    }
    else
    {
        attrPtr = catcache[r].attrList;

        for(;attrPtr;attrPtr=attrPtr->next)
        {
            if(attrPtr->attr.hasIndex)
            removeIndex(attrPtr);
        }

        printf("Destroyed index successfully on all attributes of relation %s\n", 
        relName);
    }
}