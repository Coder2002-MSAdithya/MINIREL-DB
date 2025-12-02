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
#include <stddef.h>
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
        printf("Relation '%s' already exists in the DB.\n", dstRelName);
        db_err_code = RELEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    if(r2 == NOTOK)
    {
        if(db_err_code == RELNOEXIST)
        {
            printf("Relation '%s' does NOT exist in the DB.\n", srcRelName);
            printCloseStrings(RELCAT_CACHE, offsetof(RelCatRec, relName), srcRelName);
        }
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
        printf("Attribute '%s' NOT present in relation '%s' of the DB.\n", attrName, srcRelName);
        printCloseStrings(ATTRCAT_CACHE, offsetof(AttrCatRec, attrName), attrName);
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

    if(r1 == NOTOK)
    {
        return ErrorMsgs(db_err_code, print_flag);
    }

    Rid recRid = INVALID_RID;
    int offset = (foundField->attr).offset;
    int size = (foundField->attr).length;
    char type = (foundField->attr).type;
    void *valuePtr = malloc(size);
    void *recPtr = malloc(recSize);

    if(!valuePtr || !recPtr)
    {
        db_err_code = MEM_ALLOC_ERROR;
        return ErrorMsgs(MEM_ALLOC_ERROR, print_flag);
    }

    if(!isValidForType(type, size, value, valuePtr))
    {
        db_err_code = INVALID_VALUE;
        return ErrorMsgs(db_err_code, print_flag);
    }

    /* Go through each record of srcRelName and filter */
    do
    {
        if(FindRec(r2, recRid, &recRid, recPtr, type, size, offset, valuePtr, operator) == NOTOK)
        {
            return ErrorMsgs(db_err_code, print_flag);
        }

        if(!isValidRid(recRid))
        {
            break;
        }

        if(InsertRec(r1, recPtr) == NOTOK)
        {
            return ErrorMsgs(db_err_code, print_flag);
        }
    } 
    while(true);

    printf("Created relation %s successfully and placed filtered tuples of %s\n", 
    dstRelName, srcRelName);
    
    return OK;
}