/************************INCLUDES*******************************/

#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/openrel.h"
#include "../include/findrec.h"
#include "../include/deleterec.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>


/*------------------------------------------------------------

FUNCTION Delete (argc, argv)

PARAMETER DESCRIPTION:
    argc → number of arguments in the command line.
    argv → array of argument strings.
    Specifications:
        argv[0] = "delete"
        argv[1] = relation name
        argv[2] = attribute name on which condition applies
        argv[3] = comparison operator (encoded integer)
        argv[4] = literal value to compare
        argv[argc] = NIL

FUNCTION DESCRIPTION:
    Implements delete operation. 
    All tuples in the given relation that satisfy the comparison criterion: 
        attribute <op> literal
    are removed from the file.  
    The operator is one of (=, !=, <, <=, >, >=), encoded as CMP_* constants.
    Incoming values are validated and converted according to the attribute catalog description.  
    For each matching tuple, DeleteRec() is invoked to clear the slot in the page, update slotmaps, update tuple counts, and maintain the freemap.
    DELETE is disallowed on catalog relations (relcat, attrcat).

ALGORITHM:
    1) Verify that the database is open.
    2) Reject attempts to delete from system catalogs.
    3) Check that the named relation exists; open it.
    4) Search for the attribute in the relation's attribute list.
        If not found, report an error.
    5) Validate the literal value for the attribute type. Construct a binary valuePtr for comparison.
    6) Initialize recRid = INVALID_RID.
    7) Repeatedly call FindRec():
        a) Find next tuple satisfying attribute <op> literal.
        b) If no valid RID returned, stop.
        c) Call DeleteRec() on the found tuple.
        d) Increment deletion counter.
    8) Free temporary buffers.
    9) Print number of deleted records.

BUGS:
    None found.

ERRORS REPORTED:
    DBNOTOPEN         – no database open
    METADATA_SECURITY – attempt to delete from relcat/attrcat
    RELNOEXIST        – relation does not exist
    ATTRNOEXIST       – attribute not found in relation schema
    INVALID_VALUE     – literal not valid for attribute type
    MEM_ALLOC_ERROR   – memory allocation failure
    (others propagated from FindRec/DeleteRec)

GLOBAL VARIABLES MODIFIED:
    db_err_code
    Catalog information for the relation (numRecs)
    Freemap entries (via DeleteRec)
    Buffer pages containing affected tuples

IMPLEMENTATION NOTES:
    • delete uses FindRec() to implement conditional filtering; the entire relation is scanned in RID order.
    • DeleteRec() manages slotmap updates, dirty flags, and page/freemap maintenance.
    • Comparison operators are integer codes mapped to CMP_*.
    • Caller is responsible for supplying valid operator codes.

------------------------------------------------------------*/

int Delete(int argc, char **argv)
{
    if(!db_open)
    {
        db_err_code = DBNOTOPEN;
        return ErrorMsgs(db_err_code, print_flag);
    }

    char *relName = argv[1];
    char *attrName = argv[2];
    int operator = *(int *)argv[3];
    char *value = argv[4];

    int recsAffected = 0;

    if(strncmp(relName, RELCAT, RELNAME) == OK)
    {
        printf("CANNOT delete record from relcat.\n");
        db_err_code = METADATA_SECURITY;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if(strncmp(relName, ATTRCAT, RELNAME) == OK)
    {
        printf("CANNOT delete record from attrcat.\n");
        db_err_code = METADATA_SECURITY;
        return ErrorMsgs(db_err_code, print_flag);
    }

    int r = OpenRel(relName);

    if(r == NOTOK)
    {
        printf("Relation '%s' does NOT exist in the DB.\n", relName);
        printCloseStrings(RELCAT_CACHE, offsetof(RelCatRec, relName), relName, NULL);
        db_err_code = RELNOEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    bool found = false;
    AttrDesc *ptr = catcache[r].attrList;
    int recSize = catcache[r].relcat_rec.recLength;
    
    for(;ptr;ptr=ptr->next)
    {
        if(strncmp((ptr->attr).attrName, attrName, ATTRNAME) == OK)
        {
            found = true;
            Rid recRid = INVALID_RID;
            char type = (ptr->attr).type[0];
            int offset = (ptr->attr).offset;
            int size = (ptr->attr).length;
            void *recPtr = malloc(recSize);
            void *valuePtr = malloc(size);

            if(!recPtr || !valuePtr)
            {
                db_err_code = MEM_ALLOC_ERROR;
                return ErrorMsgs(db_err_code, print_flag);
            }

            if(!isValidForType(type, size, value, valuePtr))
            {
                printf("'%s' is an INVALID literal for TYPE %s.\n", 
                value, type == 'i' ? "INTEGER" : "FLOAT");
                db_err_code = INVALID_VALUE;
                return ErrorMsgs(db_err_code, print_flag);
            }

            do
            {
                if(FindRec(r, recRid, &recRid, recPtr, type, size, offset, valuePtr, operator) == NOTOK)
                {
                    return ErrorMsgs(db_err_code, print_flag);
                }

                if(!isValidRid(recRid))
                {
                    break;
                }

                if(DeleteRec(r, recRid) == NOTOK)
                {
                    return ErrorMsgs(db_err_code, print_flag);
                }
                recsAffected++;
            } 
            while(true);

            free(recPtr);
            free(valuePtr);
            break;
        }
    }

    if(!found)
    {
        printf("Attribute '%s' NOT present in relation '%s' of the DB.\n", attrName, relName);
        printCloseStrings(ATTRCAT_CACHE, offsetof(AttrCatRec, attrName), attrName, relName);
        db_err_code = ATTRNOEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    printf("%d records successfully deleted from relation %s\n", recsAffected, relName);

    return OK;
}