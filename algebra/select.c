#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/openrel.h"
#include "../include/findrel.h"
#include "../include/findrec.h"
#include "../include/insertrec.h"
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
        db_err_code = RELNOEXIST;
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

    /*Create the result relation with 
    the same set of attributes as source relation*/
    // Make an array of Command line arguments for the Create function
    int numAttrs = catcache[r2].relcat_rec.numAttrs;
    int recSize = catcache[r2].relcat_rec.recLength;
    ptr = catcache[r2].attrList;
    int i = 2;

    // Prepare arguments for Create()
    // "create", dstRelName, and (name, format) for each attr
    int createArgc = 2 + 2 * numAttrs;
    char **createArgv = (char **)malloc(createArgc * sizeof(char *));
    createArgv[0] = strdup("_create");
    createArgv[1] = strdup(dstRelName);

    for(;ptr;ptr=ptr->next)
    {
        char *attrName = (ptr->attr).attrName;
        char type = (ptr->attr).type;
        int length = (ptr->attr).length;

        createArgv[i++] = strdup(attrName);

        // Attribute format
        char format[5];

        switch (type)
        {
            case 'i':
                strcpy(format, "i");
                break;
            case 'f':
                strcpy(format, "f");
                break;
            case 's':
                sprintf(format, "s%d", length);
                break;
            default:
                strcpy(format, "");
                break;
        }

        createArgv[i++] = strdup(format);
    }

    // NIL terminator
    createArgv[i] = NULL;

    // Create the new relation
    int cStatus = Create(createArgc, createArgv);

    // ---- Free allocated memory ----
    for(int j = 0; j < createArgc; j++)
    {
        free(createArgv[j]);
    }
    free(createArgv);

    if(cStatus != OK)
    {
        return NOTOK;
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
    
    return OK;
}