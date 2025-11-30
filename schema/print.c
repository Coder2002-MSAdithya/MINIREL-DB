/************************INCLUDES*******************************/
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/openrel.h"
#include "../include/getnextrec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/************************LOCAL DEFINES**************************/
/* assume all required headers, types, and globals (db_open, db_err_code, etc.) are defined */
static void printSeparator(AttrDesc *attrList, int *colWidths);
static void printHeader(AttrDesc *attrList, int *colWidths);


/*------------------------------------------------------------

FUNCTION Print (argc, argv)

PARAMETER DESCRIPTION:
    argc  → number of command arguments
    argv  → command argument vector
            argv[0] = "print"
            argv[1] = relation name
            argv[argc] = NIL

FUNCTION DESCRIPTION:
    Prints all tuples of a specified relation in a formatted, tabular output. 
    The function: 
        - opens the relation. 
        - inspects its attribute catalog entries. 
        - computes dynamically sized column widths. 
        - prints a header. 
        - iterates through all records in physical order using GetNextRec().
        - prints values with appropriate alignment based on attribute type.

ALGORITHM:
    1. Verify that a database is open.
    2. Validate argument count.
    3. Attempt to open the relation using OpenRel().
       - Report RELNOEXIST if the relation does not exist.
    4. Count attributes by traversing catcache[r].attrList.
    5. Allocate an array of column widths:
        - For each attribute, determine width based on:
            a. attribute name length
            b. display width for its data type
        - Store final width (with padding) in colWidths[].
    6. Print header separator and column titles using helper functions.
    7. Allocate a buffer of size recLength to hold each tuple.
    8. Set startRid = INVALID_RID and repeatedly call GetNextRec():
        a. If no valid RID is returned, stop iteration.
        b. For each attribute in the record:
            - Compute pointer to its data via offset.
            - Decode based on type:
                int  → right-aligned integer
                float → right-aligned with 2 decimals
                string → left-aligned, trimmed of trailing nulls/spaces
            - Print in a fixed-width column.
    9. After all tuples are printed:
        - Print closing separator.
        - Print row count summary.
   10. Free allocated buffers and return OK.

ERRORS REPORTED:
    DBNOTOPEN
    ARGC_INSUFFICIENT
    TOO_MANY_ARGS
    RELNOEXIST
    MEM_ALLOC_ERROR

GLOBAL VARIABLES MODIFIED:
    db_err_code

------------------------------------------------------------*/

/* ============================================================= */
int Print(int argc, char **argv)
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
    int r = OpenRel(relName);

    if(r == NOTOK)
    {
        db_err_code = RELNOEXIST;
        return ErrorMsgs(db_err_code, print_flag);
    }

    printf("OK, printing relation %s\n\n", relName);

    /* --------- Count attributes --------- */
    int attrCount = 0;
    for(AttrDesc *p = catcache[r].attrList; p; p = p->next)
        attrCount++;

    if(attrCount == 0)
    {
        printf("(Empty relation)\n");
        return OK;
    }

    /* --------- Allocate column width array --------- */
    int *colWidths = (int *)malloc(sizeof(int) * attrCount);
    if(!colWidths)
    {
        return ErrorMsgs(MEM_ALLOC_ERROR, print_flag);
    }

    /* --------- Determine column widths --------- */
    int idx = 0;
    for(AttrDesc *p = catcache[r].attrList; p; p = p->next, idx++)
    {
        int nameLen = (int)strlen(p->attr.attrName);
        int dataLen;
        switch(p->attr.type)
        {
            case 'i': dataLen = 11; break;  // enough for 32-bit int
            case 'f': dataLen = 12; break;  // float with decimals
            case 's': dataLen = p->attr.length; break;
            default:  dataLen = 10; break;
        }

        int colWidth = (nameLen > dataLen ? nameLen : dataLen);
        colWidth += 2; // padding for spacing
        colWidths[idx] = colWidth;
    }

    /* --------- Print header --------- */
    printHeader(catcache[r].attrList, colWidths);

    /* --------- Print records --------- */
    Rid startRid = INVALID_RID;
    int recSize = catcache[r].relcat_rec.recLength;
    void *recPtr = malloc(recSize);
    if(!recPtr)
    {
        free(colWidths);
        return ErrorMsgs(MEM_ALLOC_ERROR, print_flag);
    }

    int rowCount = 0;

    while(true)
    {
        int status = GetNextRec(r, startRid, &startRid, recPtr);
        if(status == NOTOK || !isValidRid(startRid))
            break;

        printf("|");
        idx = 0;

        for(AttrDesc *p = catcache[r].attrList; p; p = p->next, idx++)
        {
            AttrCatRec *ac = &(p->attr);
            char *attrPtr = (char *)recPtr + ac->offset;
            char buf[1024] = {0};

            switch(ac->type)
            {
                case 'i': {
                    int ival;
                    memcpy(&ival, attrPtr, sizeof(int));
                    snprintf(buf, sizeof(buf), "%d", ival);
                    printf(" %*s |", colWidths[idx] - 2, buf); // right align
                    break;
                }

                case 'f': {
                    float fval;
                    memcpy(&fval, attrPtr, sizeof(float));
                    snprintf(buf, sizeof(buf), "%.2f", fval);
                    printf(" %*s |", colWidths[idx] - 2, buf); // right align
                    break;
                }

                case 's': {
                    int len = ac->length;
                    char *temp = (char *)malloc(len + 1);
                    memcpy(temp, attrPtr, len);
                    temp[len] = '\0';

                    // Trim trailing nulls/spaces
                    for(int j = len - 1; j >= 0 && (temp[j] == '\0' || temp[j] == ' '); j--)
                        temp[j] = '\0';

                    snprintf(buf, sizeof(buf), "%s", temp);
                    printf(" %-*s |", colWidths[idx] - 2, buf); // left align
                    free(temp);
                    break;
                }

                default:
                    snprintf(buf, sizeof(buf), "(unknown)");
                    printf(" %-*s |", colWidths[idx] - 2, buf);
                    break;
            }
        }

        printf("\n");
        rowCount++;
    }

    /* --------- Footer --------- */
    printSeparator(catcache[r].attrList, colWidths);
    printf("%d row%s in set\n", rowCount, (rowCount == 1 ? "" : "s"));

    free(recPtr);
    free(colWidths);
    return OK;
}

/* ============================================================= */
/* --------- Helper functions (no nesting) --------- */

static void printSeparator(AttrDesc *attrList, int *colWidths)
{
    int idx = 0;
    printf("+");
    for(AttrDesc *p = attrList; p; p = p->next, idx++)
    {
        for(int i = 0; i < colWidths[idx]; i++)
            printf("-");
        printf("+");
    }
    printf("\n");
}

static void printHeader(AttrDesc *attrList, int *colWidths)
{
    printSeparator(attrList, colWidths);
    printf("|");
    int idx = 0;
    for(AttrDesc *p = attrList; p; p = p->next, idx++)
    {
        printf(" %-*s |", colWidths[idx] - 2, p->attr.attrName);
    }
    printf("\n");
    printSeparator(attrList, colWidths);
}