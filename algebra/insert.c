#include "../include/defs.h"
#include "../include/globals.h"
#include "../include/error.h"
#include "../include/openrel.h"
#include "../include/helpers.h"
#include "../include/insertrec.h"
#include "../include/getnextrec.h"
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


/*------------------------------------------------------------

FUNCTION Insert (argc, argv)

PARAMETER DESCRIPTION:
    argc → number of command arguments.
    argv → array of argument strings.
    Specifications:
        argv[0] = "insert" or "_insert"
        argv[1] = relation name
        argv[2] = attribute name 1
        argv[3] = attribute value 1
        ...
        argv[argc-2] = attribute name N
        argv[argc-1] = attribute value N
        argv[argc]   = NIL

FUNCTION DESCRIPTION:
    Implements insert operation.
    The routine constructs a binary record from ASCII literals, validating each attribute-value pair according to the schema.
    The process includes:
        • Rejecting inserts into catalog relations (relcat, attrcat)
        • Ensuring the target relation exists
        • Ensuring ALL attributes of the relation are supplied
        • Validating literals:
            - integer → must pass isValidInteger(), stored via memcpy
            - float   → must pass isValidFloat(), stored via memcpy
            - string  → copied up to attribute length, NULL-terminated
        • Checking for repeated attribute names
        • Checking for duplicate tuples via full sequential scan
        • Invoking InsertRec() to place the tuple into the file.
    The relation catalog (catcache[r].attrList) supplies offsets, lengths, and type information for each attribute.

ALGORITHM:
    1) Verify that a database is open.
    2) Disallow inserts on RELCAT and ATTRCAT (metadata-protected).
    3) Open the target relation with OpenRel().
    4) Allocate a zero-filled record buffer sized to recLength.
    5) For each (attrName, value) pair:
        a) Locate attribute descriptor in attrList.
        b) Validate literal matches required type.
        c) Convert → store at attribute offset.
    6) Check that no attribute name appears twice.
    7) Check attribute completeness:
        (#supplied attributes) == (#schema attributes)
    8) Duplicate detection:
        a) Scan relation using GetNextRec().
        b) Compare ALL attributes (compareVals or direct checks).
        c) If exact match found → reject with DUP_ROWS.
    9) If unique, call InsertRec() to insert the tuple.
    10) Print success if called interactively (“insert” vs “_insert”).

BUGS:
    None found.

ERRORS REPORTED:
    DBNOTOPEN         – no database currently open
    METADATA_SECURITY – attempted modification of catalog tables
    RELNOEXIST        – relation does not exist
    ATTRNOEXIST       – attribute not found in schema
    INVALID_VALUE     – literal invalid for declared type
    DUP_ATTR_INSERT   – same attribute repeated in command
    ATTR_SET_INVALID  – missing or extra attributes
    DUP_ROWS          – duplicate tuple detected
    MEM_ALLOC_ERROR   – memory allocation failed
    REC_INS_ERR       – InsertRec() failed

GLOBAL VARIABLES MODIFIED:
    db_err_code       – global error state
    buffer[]          – via InsertRec (page updates)
    catcache[]        – via InsertRec (numRecs metadata)

IMPLEMENTATION NOTES:
    - Uses isValidInteger(), isValidFloat(), isValidForType(), compareVals(), GetNextRec(), and InsertRec().
    - InsertRec() performs slot allocation, page writes, freemap updates, and relcat metadata increments.
    - "_insert" form suppresses user-facing print messages.

------------------------------------------------------------*/


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
        printf("CANNOT insert records into relcat.\n");
        db_err_code = METADATA_SECURITY;
        return ErrorMsgs(db_err_code, print_flag && flag);
    }

    if(strncmp(relName, ATTRCAT, RELNAME) == OK)
    {
        printf("CANNOT insert records into attrcat.\n");
        db_err_code = METADATA_SECURITY;
        return ErrorMsgs(db_err_code, print_flag && flag);
    }

    int r = OpenRel(relName);
    int numAttrs = catcache[r].relcat_rec.numAttrs;

    if(r == NOTOK)
    {
        printf("Relation '%s' does NOT exist in the DB.\n", relName);
        printCloseStrings(RELCAT_CACHE, offsetof(RelCatRec, relName), relName, NULL);
        db_err_code = RELNOEXIST;
        return ErrorMsgs(RELNOEXIST, print_flag && flag);
    }

    int recLength = catcache[r].relcat_rec.recLength;
    void *newRecord = calloc(recLength, 1);

    if(!newRecord)
    {
        db_err_code = MEM_ALLOC_ERROR;
        return ErrorMsgs(db_err_code, print_flag);
    }

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
                char type = (ptr->attr).type[0];
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
                        printf("'%s' is NOT a valid INTEGER literal.\n", value);
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
                        printf("'%s' is NOT a valid FLOAT literal.\n", value);
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
            printf("Attribute '%s' does NOT exist in relation %s of DB.\n", argv[i], relName);
            printCloseStrings(ATTRCAT_CACHE, offsetof(AttrCatRec, attrName), argv[i], relName);
            db_err_code = ATTRNOEXIST;
            return ErrorMsgs(db_err_code, print_flag && flag);
        }
    }

    /* Check for same attribute name repeated multiple times */
    for(int i=2; i<argc; i+=2)
    {
        for(int j=i+2; j<argc; j+=2)
        {
            if(strncmp(argv[i], argv[j], ATTRNAME) == OK)
            {
                printf("Attribute %s is duplicate in the INSERT statement.\n", argv[i]);
                db_err_code = DUP_ATTR_INSERT;
                return ErrorMsgs(db_err_code, DUP_ATTR_INSERT);
            }
        }
    }

    /* We also need to check whether attribute set is correct */
    int numInsAttrs = (argc - 2) >> 1;
    if(numInsAttrs != numAttrs)
    {
        return ErrorMsgs(ATTR_SET_INVALID, print_flag);
    }

    /*We need to check for duplicate tuples also*/
    Rid recId = INVALID_RID;
    void *recPtr = malloc(recLength);
    do
    {
        if(GetNextRec(r, recId, &recId, recPtr) == NOTOK)
        {
            return ErrorMsgs(db_err_code, print_flag);
        }

        if(!isValidRid(recId))
        {
            break;
        }

        AttrDesc *ptr = catcache[r].attrList;
        bool matchingTupleFound = true;

        for(;ptr;ptr=ptr->next)
        {
            char type = (ptr->attr).type[0];
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
                if(!compareVals(s1, s2, 's', size, CMP_EQ))
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