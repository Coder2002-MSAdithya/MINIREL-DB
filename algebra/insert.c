#include "../include/defs.h"
#include "../include/globals.h"
#include "../include/error.h"
#include "../include/openrel.h"
#include "../include/helpers.h"
#include "../include/insertrec.h"
#include "../include/getnextrec.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* Here we assume that the right number of arguments are being passed */
int Insert(int argc, char **argv)
{
    bool flag = (strcmp(argv[0], "insert") == OK);

    if(!db_open)
    {
        db_err_code = DBNOTOPEN;
        return ErrorMsgs(db_err_code, DBNOTOPEN);
    }

    char *relName = argv[1];

    if(strncmp(relName, RELCAT, RELNAME) == OK)
    {
        db_err_code = METADATA_SECURITY;
        return ErrorMsgs(db_err_code, print_flag && flag);
    }

    if(strncmp(relName, ATTRCAT, RELNAME) == OK)
    {
        db_err_code = METADATA_SECURITY;
        return ErrorMsgs(db_err_code, print_flag && flag);
    }

    int r = OpenRel(relName);

    if(r == NOTOK)
    {
        db_err_code = RELNOEXIST;
        return ErrorMsgs(RELNOEXIST, print_flag && flag);
    }

    int recLength = catcache[r].relcat_rec.recLength;
    void *newRecord = malloc(recLength);

    for(int i=2; i<argc; i+=2)
    {
        bool found = false;
        AttrDesc *ptr = catcache[r].attrList;

        for(;ptr;ptr=ptr->next)
        {
            if(strncmp((ptr->attr).attrName, argv[i], ATTRNAME) == OK)
            {
                found = true;
                char *value = argv[i+1];
                char type = (ptr->attr).type;
                int offset = (ptr->attr).offset;
                int size = (ptr->attr).length;

                if(type == 'i')
                {
                    /* Check if the value string is a valid integer literal 
                    (may have negatives)*/
                    /* If so, then set *(int *)(newRecordPtr + offset) = atoi(value) */
                    /* Else set the db_err_code global variable to INVALID_VALUE
                    and return ErrorMsgs(db_err_code, INVALID_VALUE) */
                    if(!isValidInteger(value))
                    {
                        db_err_code = INVALID_VALUE;
                        free(newRecord);
                        return ErrorMsgs(INVALID_VALUE, print_flag && flag);
                    }

                    int intval = atoi(value);
                    memcpy((char *)newRecord + offset, &intval, sizeof(int));
                }
                else if(type == 'f')
                {
                    /* Check if the value string is a valid floating point literal 
                    (may have negatives)*/
                    /* If so, then set *(float *)(newRecordPtr + offset) = atof(value) (or parse it as float of 4 bytes) */
                    /* Else set the db_err_code global variable to INVALID_VALUE
                    and return ErrorMsgs(db_err_code, INVALID_VALUE) */
                    if(!isValidFloat(value))
                    {
                        db_err_code = INVALID_VALUE;
                        free(newRecord);
                        return ErrorMsgs(INVALID_VALUE, print_flag && flag);
                    }

                    float fval = atof(value);
                    memcpy((char *)newRecord + offset, &fval, sizeof(float));
                }
                else if(type == 's')
                {
                    // Copy string only upto the attribute capacity
                    strncpy((char *)newRecord+offset, value, size);

                    // Add NULL terminator if value too long ...
                    if(strlen(value) >= size)
                    {
                        ((char *)newRecord)[offset + size - 1] = '\0';
                    }
                }
            }
        }

        if(!found)
        {
            db_err_code = ATTRNOEXIST;
            return ErrorMsgs(ATTRNOEXIST, print_flag && flag);
        }
    }

    /* Check for same attribute name repeated multiple times */
    for(int i=2; i<argc; i+=2)
    {
        for(int j=i+2; j<argc; j+=2)
        {
            if(strncmp(argv[i], argv[j], ATTRNAME) == OK)
            {
                db_err_code = DUP_ATTR_INSERT;
                return ErrorMsgs(db_err_code, DUP_ATTR_INSERT);
            }
        }
    }

    /*We need to check for duplicate tuples also*/
    Rid recId = INVALID_RID;
    void *recPtr = malloc(recLength);
    do
    {
        GetNextRec(r, recId, &recId, recPtr);

        if(!isValidRid(recId))
        {
            break;
        }

        AttrDesc *ptr = catcache[r].attrList;
        bool matchingTupleFound = true;

        for(;ptr;ptr=ptr->next)
        {
            char type = (ptr->attr).type;
            int offset = (ptr->attr).offset;
            int size = (ptr->attr).length;

            if(type == 'i')
            {
                /* Compare *(int *)((char *)recPtr + offset) and 
                *(int *)((char *)recPtr + offset) 
                as integers */
                /*If they are NOT equal, then set matchingTupleFound as false*/
                if(*(int *)((char *)recPtr + offset) != *(int *)((char *)newRecord + offset))
                {
                    matchingTupleFound = false;
                    break;
                }   
            }
            else if(type == 'f')
            {
                /* Compare *(float *)((char *)recPtr + offset) and 
                *(float *)((char *)recPtr + offset) 
                as integers */
                /*If they are NOT equal, then set matchingTupleFound as false*/
                float f1 = *(float *)((char *)recPtr + offset);
                float f2 = *(float *)((char *)newRecord + offset);
                float diff = f1 - f2;
                if(!compareVals(&f1, &f2, 'f', sizeof(float), CMP_EQ))
                {
                    matchingTupleFound = false;
                }
            }
            else if(type == 's')
            {
                /* Compare ((char *)recPtr + offset) and ((char *)recPtr + offset) 
                as size length strings*/
                /* If NOT equal, then set matchingTupleFound = false */
                char *s1 = (char *)recPtr + offset;
                char *s2 = (char *)newRecord + offset;
                if(compareVals(s1, s2, 's', size, CMP_EQ))
                {
                    matchingTupleFound = false;
                    break;
                }
            }
        }

        if(matchingTupleFound)
        {
            db_err_code = DUP_ROWS;
            return ErrorMsgs(db_err_code, print_flag && flag);
        }
    } 
    while(1);
    
    /* Free the sequential search record pointer */
    free(recPtr);

    /* Insert the new record in the relation */
    if(InsertRec(r, newRecord) == OK)
    {
        if(flag)
        printf("Inserted record successfully into %s\n", relName);
        free(newRecord);
        return OK;
    }
    else
    {
        free(newRecord);
        db_err_code = REC_INS_ERR;
        return ErrorMsgs(db_err_code, print_flag && flag);
    }
}