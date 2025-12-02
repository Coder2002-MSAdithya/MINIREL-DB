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
        printf("CANNOT delete record fron relcat.\n");
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
            char type = (ptr->attr).type;
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