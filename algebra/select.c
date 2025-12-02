#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/openrel.h"
#include "../include/findrel.h"
#include "../include/findrec.h"
#include "../include/insertrec.h"
#include "../include/createrel.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

int Select(int argc, char **argv)
{
    if(!db_open)
    {
        db_err_code = DBNOTOPEN;
        return ErrorMsgs(db_err_code, print_flag);
    }

    char *dstRelName = argv[1];
    char *srcRelName = argv[2];
    char *attrName = argv[3];
    int operator = *(int *)(argv[4]);
    char *value = argv[5];

    bool attrFound = false;

    int r1 = FindRel(dstRelName);
    int r2 = OpenRel(srcRelName);

    if(r1)
    {
        db_err_code = RELEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if(r2 == NOTOK)
    {
        return ErrorMsgs(db_err_code, print_flag);
    }

    AttrDesc *ptr = catcache[r2].attrList;
    AttrDesc *foundField = NULL;

    for(;ptr;ptr=ptr->next)
    {
        if(strncmp((ptr->attr).attrName, attrName, ATTRNAME) == OK)
        {
            attrFound = true;
            foundField = ptr;
            break;
        }
    }

    if(!attrFound)
    {
        db_err_code = ATTRNOEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }
    
    int recSize = catcache[r2].relcat_rec.recLength;

    if(CreateRel(dstRelName, r2) == NOTOK)
    {
        return ErrorMsgs(db_err_code, print_flag);
    }
    
    /* Insert records into result relation that satisfy criteria */
    r1 = OpenRel(dstRelName);
    Rid recRid = INVALID_RID;
    int offset = (foundField->attr).offset;
    int size = (foundField->attr).length;
    char type = (foundField->attr).type;
    void *valuePtr = malloc(size);
    void *recPtr = malloc(recSize);

    if(!isValidForType(type, size, value, valuePtr))
    {
        db_err_code = INVALID_VALUE;
        return ErrorMsgs(db_err_code, print_flag);
    }

    /* Go through each record of srcRelName and filter */
    do
    {
        FindRec(r2, recRid, &recRid, recPtr, type, size, offset, valuePtr, operator);

        if(!isValidRid(recRid))
        {
            break;
        }

        InsertRec(r1, recPtr);
    } 
    while(true);

    printf("Created relation %s successfully and placed filtered tuples of %s\n", 
    dstRelName, srcRelName);
    
    return OK;
}