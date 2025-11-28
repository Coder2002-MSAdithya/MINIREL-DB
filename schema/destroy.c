/************************INCLUDES*******************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/closerel.h"
#include "../include/deleterec.h"
#include "../include/findrec.h"


/*------------------------------------------------------------

FUNCTION Destroy (argc, argv)

PARAMETER DESCRIPTION:
    argc → number of command line arguments.
    argv → array of strings:
            argv[0] = "destroy"
            argv[1] = relation name to delete

FUNCTION DESCRIPTION:
    Deletes a relation from the database. The function:
      - verifies the DB is open,
      - performs argument validation,
      - prevents deleting system catalogs (relcat/attrcat),
      - finds the relation's entry in relcat,
      - closes relation if currently open,
      - deletes relcat entry for the relation,
      - deletes all attribute catalog entries for the relation,
      - deletes underlying relation file from disk.

ALGORITHM:
    1. Ensure DB is open and argc = 2.
    2. Reject attempts to destroy system catalogs (metadata security).
    3. Locate relation's relcat record via FindRec.
    4. If relation is open, call CloseRel(relNum).
    5. Delete the relcat record via DeleteRec.
    6. Repeatedly find and delete attrcat entries belonging to the relation.
    7. Remove the relation file using remove().
    8. Report success or an appropriate error code.

ERRORS REPORTED:
    DBNOTOPEN
    ARGC_INSUFFICIENT
    TOO_MANY_ARGS
    METADATA_SECURITY
    RELNOEXIST
    FILESYSTEM_ERROR

GLOBAL VARIABLES MODIFIED:
    - Catalog contents: relcat and attrcat (through DeleteRec)
    - catcache entries may change (CloseRel)
    - db_err_code

------------------------------------------------------------*/

int Destroy(int argc, char *argv[])
{
    if(!db_open)
    {
        db_err_code = DBNOTOPEN;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if(argc < 2)
    {
        db_err_code = ARGC_INSUFFICIENT;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if(argc > 2)
    {
        db_err_code = TOO_MANY_ARGS;
        return ErrorMsgs(db_err_code, print_flag);
    }

    char *relName = argv[1];
    void *relCatRecPtr = malloc(sizeof(RelCatRec));
    void *attrCatRecPtr = malloc(sizeof(AttrCatRec));
    Rid startRid = INVALID_RID;

    if(strncmp(relName, RELCAT, RELNAME) == OK)
    {
        db_err_code = METADATA_SECURITY;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if(strncmp(relName, ATTRCAT, RELNAME) == OK)
    {
        db_err_code = METADATA_SECURITY;
        return ErrorMsgs(db_err_code, print_flag);
    }

    int status = FindRec(RELCAT_CACHE, startRid, &startRid, relCatRecPtr, 's', RELNAME, offsetof(RelCatRec, relName), relName, CMP_EQ);

    if(status == NOTOK)
    {
        db_err_code = UNKNOWN_ERROR;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if(!isValidRid(startRid))
    {
        db_err_code = RELNOEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    int r = FindRelNum(relName);

    if(r != NOTOK)
    {
        CloseRel(r);
    }

    DeleteRec(RELCAT_CACHE, startRid);
    startRid = INVALID_RID;

    do
    {
        status = FindRec(ATTRCAT_CACHE, startRid, &startRid, attrCatRecPtr, 's', RELNAME, offsetof(AttrCatRec, relName), relName, CMP_EQ);
        
        if(status == NOTOK)
        {
            db_err_code = UNKNOWN_ERROR;
            free(relCatRecPtr);
            free(attrCatRecPtr);
            return ErrorMsgs(db_err_code, print_flag);
        }

        if(isValidRid(startRid))
        {
            DeleteRec(ATTRCAT_CACHE, startRid);
        }
        else break;
    } 
    while(1);
    
    if(remove(relName) == OK)
    {
        printf("Relation %s destroyed successfully.\n", relName);
    }
    else
    {
        db_err_code = FILESYSTEM_ERROR;
        return ErrorMsgs(db_err_code, print_flag);
    }

    free(relCatRecPtr);
    free(attrCatRecPtr);

    return OK;
}
