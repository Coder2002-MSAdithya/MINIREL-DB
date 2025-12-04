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


/*------------------------------------------------------------

FUNCTION Select (argc, argv)

PARAMETER DESCRIPTION:
    argc → number of command arguments
    argv → argument vector
    Specifications:
        argv[0] = "select"
        argv[1] = destination relation name (new relation)
        argv[2] = source relation name
        argv[3] = attribute name in source relation
        argv[4] = comparison operator (integer-encoded)
        argv[5] = literal value
        argv[argc] = NIL

FUNCTION DESCRIPTION:
    Implements the relational selection operator σ.
    This routine creates a new relation (dstRelName) having exactly the same schema as the source relation (srcRelName). 
    It then scans every record of the source relation, evaluates the selection predicate:
        srcRel[attrName] <op> literal
    and inserts each matching tuple into the destination relation.
    Attribute-type validation, literal conversion, and comparison are performed according to the attribute’s format stored in the catalog (integer, float, string).
    The routine fails if:
        • source relation does not exist,
        • destination relation already exists,
        • attribute does not exist,
        • the literal is not valid for the attribute type.

ALGORITHM:
    1) Ensure that a database is open; otherwise signal error.
    2) Extract arguments: dstRelName, srcRelName, attrName, operator code, literal value.
    3) Check that destination relation does not already exist.
    4) Open source relation via OpenRel():
        if it does not exist → RELNOEXIST.
    5) Search the source relation’s attribute list for attrName:
        if not found → ATTRNOEXIST.
    6) Determine the attribute's offset, length, and type.
    7) Create a new relation (dstRelName) using CreateRel() with the same schema as the source relation.
    8) Open the new destination relation using OpenRel().
    9) Convert the literal value into typed form using isValidForType(); allocate storage for casted value.
        if invalid → INVALID_VALUE.
    10) Sequentially scan the source relation:
        a) Call FindRec() to locate the next matching tuple.
        b) If foundRid is INVALID_RID → end of scan.
        c) If a matching tuple is found, insert it into the destination relation using InsertRec().
    11) Print success message.
    12) Return OK.

BUGS:
    • Does not prevent selection into the same relation name if both src and dst are identical (handled earlier by FindRel()).
    • Does not check for duplicate tuples in the destination, which is acceptable for MINIREL’s select semantics.

ERRORS REPORTED:
       DBNOTOPEN
       RELEXIST (destination already exists)
       RELNOEXIST (source does not exist)
       ATTRNOEXIST
       INVALID_VALUE (literal incompatible with attribute type)
       MEM_ALLOC_ERROR
       REC_INS_ERR (via InsertRec)
       UNKNOWN_ERROR (via FindRec)

GLOBAL VARIABLES MODIFIED:
    db_err_code
    Catalog entries for dstRelName (through CreateRel)
    Physical heap pages for dstRelName (InsertRec)

IMPLEMENTATION NOTES:
    • The operator argument is expected to be an integer code corresponding to comparison constants (CMP_EQ, CMP_LT, etc.).
    • Select creates a full relation copy structurally identical to the source relation; projection is a separate operator.
    • Uses FindRec iteratively to locate each successive match, enabling efficient cursor-style traversal.

------------------------------------------------------------*/

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
            printCloseStrings(RELCAT_CACHE, offsetof(RelCatRec, relName), srcRelName, NULL);
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
        printCloseStrings(ATTRCAT_CACHE, offsetof(AttrCatRec, attrName), attrName, srcRelName);
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
    char type = (foundField->attr).type[0];
    void *valuePtr = malloc(size);
    void *recPtr = malloc(recSize);

    if(!valuePtr || !recPtr)
    {
        db_err_code = MEM_ALLOC_ERROR;
        return ErrorMsgs(MEM_ALLOC_ERROR, print_flag);
    }

    if(!isValidForType(type, size, value, valuePtr))
    {
        printf("'%s' is an INVALID literal for TYPE %s.\n", 
        value, type == 'i' ? "INTEGER" : "FLOAT");
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