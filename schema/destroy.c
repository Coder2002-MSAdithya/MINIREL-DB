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
