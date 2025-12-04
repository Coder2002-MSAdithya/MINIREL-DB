/************************INCLUDES*******************************/

#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/openrel.h"
#include "../include/findrel.h"
#include "../include/getnextrec.h"
#include "../include/createfromattrlist.h"
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


/*------------------------------------------------------------

FUNCTION InsertProjectedRecFromLL (dstRelName, head, recPtr)

PARAMETER DESCRIPTION:
    dstRelName → name of destination relation into which the projected tuple is to be inserted.
    head       → pointer to a linked list of AttrDesc nodes, each describing an attribute selected for projection (in final schema order).
    recPtr     → pointer to the full source record from which projected attribute values must be extracted.

FUNCTION DESCRIPTION:
    Helper routine used exclusively by the Project operator.
    It constructs a synthetic argv[] argument list that matches the format expected by Insert(), and calls Insert() to insert a projected tuple into dstRelName.
    The routine:
    • Counts attributes in the projection list.
    • Allocates a temporary argv[] array of size 2+2*k where k = number of projected attributes.
    • For each attribute:
        – copies attribute name,
        – extracts the attribute value from recPtr via (offset, type, length),
        – formats the value into string form compatible with Insert() ("123", "12.5", "hello", …).
    • Invokes Insert(insertArgc, insertArgv).
    • Frees all allocated temporary memory.
    • Returns Insert()’s return code.

ALGORITHM:
    1) Validate parameters (dstRelName, head, recPtr).
    2) Count number of attributes in linked list.
    3) Allocate argv[] of size (2+2*k).
    4) Set:
        argv[0] = "_insert"
        argv[1] = dstRelName
    5) For each attribute in 'head':
        a) Copy attribute name into argv[].
        b) Extract typed value from recPtr using:
                offset = attr.offset
                type   = attr.type
                length = attr.length
        c) Convert extracted binary value into appropriate string representation:
                - int     →  sprintf("%d")
                - float   →  sprintf("%.2f")
                - string  →  strncpy up to attribute length
        d) Store the formatted string in argv[].
    6) Call Insert() using the constructed argv[].
    7) Free all allocated strings and argv[] array.
    8) Return Insert()’s return status.

BUGS:
    • Memory is allocated for each attribute; failure during formatting attempts partial cleanup but relies on Insert() not holding references afterward.
    • Does not perform duplicate elimination — Insert() handles it.
    • Assumes attribute types are strictly one of {'i','f','s'}.

ERRORS REPORTED:
    MEM_ALLOC_ERROR (local check)
    Any error Insert() reports (e.g. DUP_ROWS, REC_INS_ERR)

GLOBAL VARIABLES MODIFIED:
    None directly; Insert() performs catalog/page modifications.

IMPLEMENTATION NOTES:
    • This routine acts as a bridge between projection output and the existing Insert() command parser.
    • recPtr is expected to point to a fully populated tuple from GetNextRec().
    • Destination relation must already exist and be opened before this is called.

------------------------------------------------------------*/

int InsertProjectedRecFromLL(char *dstRelName, AttrDesc *head, void *recPtr)
{
    if (!dstRelName || !head || !recPtr)
        return NOTOK;

    /* Step 1: Count attributes */
    int attrCount = 0;
    AttrDesc *ptr = head;
    for (; ptr; ptr = ptr->next)
        attrCount++;

    /* Step 2: Allocate argv array */
    int insertArgc = 2 + 2 * attrCount;  // excluding NULL terminator
    char **insertArgv = (char **)malloc((insertArgc) * sizeof(char *));
    if (!insertArgv)
        return NOTOK;

    insertArgv[0] = strdup("_insert");
    insertArgv[1] = strdup(dstRelName);

    /* Step 3: Fill in attribute name-value pairs */
    ptr = head;
    int argIndex = 2;
    for (; ptr; ptr = ptr->next)
    {
        insertArgv[argIndex++] = strdup((ptr->attr).attrName);

        char type = (ptr->attr).type[0];
        int length = (ptr->attr).length;
        int offset = (ptr->attr).offset;
        char *valStr = NULL;

        if (type == 'i')
        {
            int val = *(int *)((char *)recPtr + offset);
            valStr = (char *)malloc(32);
            if (valStr)
                sprintf(valStr, "%d", val);
        }
        else if (type == 'f')
        {
            float val = *(float *)((char *)recPtr + offset);
            valStr = (char *)malloc(64);
            if (valStr)
                sprintf(valStr, "%.2f", val);
        }
        else if (type == 's')
        {
            char *src = (char *)recPtr + offset;
            valStr = (char *)malloc(length);
            if (valStr)
            {
                strncpy(valStr, src, length);
                valStr[length-1] = '\0';
            }
        }
        else
        {
            valStr = strdup("");
        }

        if (!valStr)
        {
            /* free everything allocated so far to avoid leaks */
            db_err_code = MEM_ALLOC_ERROR;
            for (int j = 0; j < argIndex; j++)
                free(insertArgv[j]);
            free(insertArgv);
            return NOTOK;
        }

        insertArgv[argIndex++] = valStr;
    }

    /* Step 5: Call Insert() */
    int ret = Insert(insertArgc, insertArgv);

    /* Step 6: Free all memory */
    for (int i = 0; i < argIndex; i++)
        free(insertArgv[i]);
    free(insertArgv);

    return ret;
}


/*------------------------------------------------------------

FUNCTION Project (argc, argv)

PARAMETER DESCRIPTION:
    argc → number of command-line arguments
    argv → argument vector
    Specification:
        argv[0] = "project"
        argv[1] = destination relation name
        argv[2] = source relation name
        argv[3] = first attribute to retain
        argv[4] = second attribute to retain
        ...
        argv[argc-1] = last attribute to retain
        argv[argc] = NIL

FUNCTION DESCRIPTION:
    Implements the relational projection operator π.
    The routine creates a new relation (dstRelName) whose schema consists ONLY of the attributes explicitly listed in the command. 
    The attribute descriptors of these fields are copied from the source relation’s attribute catalog into a temporary local linked list. 
    CreateFromAttrList() is then used to create a new relation with exactly these fields.
    After the schema is created, every tuple in the source relation is scanned sequentially using GetNextRec(), and a projected tuple containing only the selected fields is constructed and inserted into the destination relation using InsertProjectedRecFromLL().
    Duplicate elimination is NOT performed by this routine. (If Insert() signals DUP_ROWS, it is ignored unless it indicates a different error.)

ALGORITHM:
    1) Verify that a database is open.
    2) Extract dstRelName and srcRelName.
    3) Open the source relation via OpenRel().
        If it does not exist → RELNOEXIST.
    4) For each attribute listed in argv[3..argc-1]:
        a) Locate attribute descriptor in source schema using getAttrDesc().
        b) If not found → ATTRNOEXIST.
        c) Allocate a new AttrDesc node, copy catalog contents, append to a temporary linked list.
    5) Check whether destination relation already exists:
        if yes → RELEXIST.
    6) Call CreateFromAttrList(dstRelName, head) to construct the new relation schema.
    7) Open the new destination relation.
    8) Sequentially scan the source relation:
        a) Call GetNextRec() repeatedly to read each tuple.
        b) If rid invalid → end of scan.
        c) Construct projected record and insert it into dstRelName using InsertProjectedRecFromLL().
            Ignore DUP_ROWS (optional duplicate avoidance).
            For any other error → report via ErrorMsgs().
    9) Print success message.

BUGS:
    • No duplicate elimination is performed, although Insert() may detect exact byte-for-byte duplicates.
    • No type checking between repeated attribute names (already prevented earlier).
    • Temporary attribute list (`head`) is never freed here (expected at program exit or via schema teardown routines).

ERRORS REPORTED:
    DBNOTOPEN
    RELNOEXIST
    ATTRNOEXIST
    RELEXIST
    MEM_ALLOC_ERROR
    REC_INS_ERR (propagated from Insert())
    UNKNOWN_ERROR (from GetNextRec or helpers)

GLOBAL VARIABLES MODIFIED:
    db_err_code
    Catalog entries for dstRelName (schema creation)
    Heap pages for dstRelName (via Insert())

IMPLEMENTATION NOTES (IF ANY):
    • Projection preserves attribute order exactly as listed in the command, not necessarily in source schema order.
    • InsertProjectedRecFromLL() builds a synthetic argv[] to reuse Insert() rather than writing directly into pages.
    • The schema copy uses a deep copy of AttrCatRec but does not modify offsets; CreateFromAttrList recomputes offsets for the new relation.

------------------------------------------------------------*/

int Project(int argc, char **argv)
{
    if(!db_open)
    {
        db_err_code = DBNOTOPEN;
        return ErrorMsgs(db_err_code, print_flag);
    }

    char *dstRelName = argv[1];
    char *srcRelName = argv[2];

    int r = OpenRel(srcRelName);

    if(r == NOTOK)
    {
        if(db_err_code == RELNOEXIST)
        {
            printf("Relation '%s' does NOT exist in the DB.\n", srcRelName);
            printCloseStrings(RELCAT_CACHE, offsetof(RelCatRec, relName), srcRelName, NULL);
        }
        return ErrorMsgs(db_err_code, print_flag);
    }

    AttrDesc *head = NULL;
    AttrDesc *tail = NULL;
    int recLength = catcache[r].relcat_rec.recLength;

    for(int c = 3; c < argc; c++)
    {
        AttrDesc *ptr = getAttrDesc(r, argv[c]);

        if(!ptr)
        {
            printf("Attribute '%s' does NOT exist in relation '%s' of the DB.\n", argv[c], srcRelName);
            printCloseStrings(ATTRCAT_CACHE, offsetof(AttrCatRec, attrName), argv[c], srcRelName);
            db_err_code = ATTRNOEXIST;
            return ErrorMsgs(db_err_code, print_flag);
        }
        else
        {
            /* Add a copy of the AttrDesc node pointed to by ptr 
            to the linked list headed by head of type (AttrDesc *) */

            /* Allocate a new AttrDesc node */
            AttrDesc *newNode = (AttrDesc *)malloc(sizeof(AttrDesc));

            if (!newNode)
            {
                db_err_code = MEM_ALLOC_ERROR;
                return ErrorMsgs(db_err_code, print_flag);
            }

            /* Copy the attribute catalog record contents */
            newNode->attr = ptr->attr;

            /* Initialize next pointer */
            newNode->next = NULL;

            /* Insert at end of linked list */
            if (!head)
            {
                head = newNode;
                tail = newNode;
            }
            else
            {
                tail->next = newNode;
                tail = newNode;
            }
        }
    }

    int s = FindRel(dstRelName);

    if(s)
    {
        printf("Relation '%s' already exists in the DB.\n", dstRelName);
        db_err_code = RELEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    s = CreateFromAttrList(dstRelName, head);

    if(s != OK)
    {
        return ErrorMsgs(db_err_code, print_flag);
    }

    Rid recRid = INVALID_RID;
    void *recPtr = malloc(recLength);
    do
    {
        if(GetNextRec(r, recRid, &recRid, recPtr) == NOTOK)
        {
            return ErrorMsgs(db_err_code, print_flag);
        }

        if(!isValidRid(recRid))
        {
            break;
        }

        int sIns = InsertProjectedRecFromLL(dstRelName, head, recPtr);
        if(sIns != OK && sIns != DUP_ROWS)
        {
            return ErrorMsgs(db_err_code, print_flag);
        }
    }
    while(true);

    printf("Projected relation %s into %s successfully.\n", srcRelName, dstRelName);

    return OK;
}