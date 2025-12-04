#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../include/defs.h"
#include "../include/helpers.h"
#include "../include/globals.h"
#include "../include/readpage.h"
#include "../include/error.h"


/*--------------------------------------------------------------

FUNCTION modifyRec (foundRid, rid, recPtr, curRec, recSize)

PARAMETER DESCRIPTION:
    foundRid  → (OUT) pointer to a Rid structure that will receive the RID of the located record.
    rid       → the RID of the record currently being examined.
    recPtr    → pointer to caller-allocated buffer in which the found record will be copied.
    curRec    → pointer to a temporary buffer holding the candidate record read from disk.
    recSize   → the record length in bytes.

FUNCTION DESCRIPTION:
    This helper routine is used inside FindRec() to finalize the discovery of a qualifying record.
    When a predicate matches:
        - foundRid is set to rid.
        - recPtr is populated with the contents of curRec.

ALGORITHM:
       1) *foundRid = rid.
       2) Copy recSize bytes from curRec to recPtr.

GLOBAL VARIABLES MODIFIED:
       None.

ERRORS REPORTED:
       None.

--------------------------------------------------------------*/


void modifyRec(Rid *foundRid, Rid rid, void *recPtr, void *curRec, int recSize)
{
    *foundRid = rid;
    memcpy(recPtr, curRec, recSize);
}


/*--------------------------------------------------------------

FUNCTION compareRecs (curRec, valuePtr, attrType, attrSize, attrOffset, compOp)

PARAMETER DESCRIPTION:
    curRec     → pointer to a record image currently being tested.
    valuePtr   → pointer to the constant value used in comparison.
    attrType   → 'i', 'f', or 's' indicating INTEGER, FLOAT, or STRING.
    attrSize   → size of the attribute in bytes.
    attrOffset → byte offset of the attribute inside curRec.
    compOp     → comparison operator: CMP_EQ, CMP_NE, CMP_LT, CMP_LTE, CMP_GT, CMP_GTE.

FUNCTION DESCRIPTION:
    This function evaluates a predicate of the form:
        curRec[attr] <op> literal
    The attribute is extracted from curRec using attrOffset and interpreted based on attrType:
        For INTEGER:      compare as signed 4-byte ints.
        For FLOAT:        compare using float_cmp() with ε-based stable float comparison.
        For STRING:       fixed-length comparison using strncmp() over attrSize bytes.
    Returns TRUE (1) if the predicate holds, otherwise FALSE (0).
    Special handling exists for floating-point NaN values:
        - Ordered comparisons (<, <=, >, >=) return FALSE.
        - Only inequality (CMP_NE) returns TRUE.

ALGORITHM:
    1) Extract attribute from curRec based on attrType.
    2) Compute cmp:
        -1 if attribute < literal
        0 if attribute == literal
        1 if attribute > literal
        2 if NaN detected (float only)
    3) If cmp == 2 (NaN case):
        return (compOp == CMP_NE).
    4) For normal cases, apply compOp semantics:
        CMP_EQ:  cmp == 0
        CMP_NE:  cmp != 0
        CMP_LT:  cmp <  0
        CMP_LTE: cmp <= 0
        CMP_GT:  cmp >  0
        CMP_GTE: cmp >= 0
    5) Return 1 if predicate true, else 0.

GLOBAL VARIABLES MODIFIED:
       None.

ERRORS REPORTED:
       None.

--------------------------------------------------------------*/

int compareRecs(void *curRec, void *valuePtr, char attrType, int attrSize, int attrOffset, int compOp)
{
    int cmp = 0;   /* -1,0,1 like strcmp/float_cmp; special 2 for NaN in floats */

    switch (attrType) {
        case 'i': {
            int curVal = *(int *)((char *)curRec + attrOffset);
            int val = *(int *)valuePtr;
            if (curVal < val) cmp = -1;
            else if (curVal > val) cmp = 1;
            else cmp = 0;
            break;
        }
        case 'f': {
            double curVal = (double)(*(float *)((char *)curRec + attrOffset));
            double val = (double)(*(float *)valuePtr);
            cmp = float_cmp(curVal, val, FLOAT_REL_EPS, FLOAT_ABS_EPS);
            break;
        }
        case 's': {
            char *curVal = (char *)curRec + attrOffset;
            char *val = (char *)valuePtr;
            int s = strncmp(curVal, val, attrSize);
            if (s < 0) cmp = -1;
            else if (s > 0) cmp = 1;
            else cmp = 0;
            break;
        }
        default:
            return 0; /* unknown type -> treat as no match */
    }

    /* If float comparison produced NaN indicator: ordered comparisons are false,
       only 'not equal' should be true (NaN != anything) */
    if (cmp == 2) {
        if (compOp == CMP_NE) return 1;
        return 0;
    }

    switch (compOp) {
        case CMP_EQ:  return (cmp == 0);
        case CMP_NE:  return (cmp != 0);
        case CMP_GT:  return (cmp > 0);
        case CMP_GTE: return (cmp > 0 || cmp == 0);
        case CMP_LT:  return (cmp < 0);
        case CMP_LTE: return (cmp < 0 || cmp == 0);
        default:      return 0;
    }
}


/*--------------------------------------------------------------

FUNCTION FindRec (relNum, startRid, foundRid, recPtr, attrType, attrSize, attrOffset, valuePtr, compOp)

PARAMETER DESCRIPTION:
    relNum     → relation number (index in catcache[]).
    startRid   → the RID from which the search continues.
    foundRid   → (OUT) receives RID of the next record that satisfies the selection predicate, or { -1, -1 } if none.
    recPtr     → buffer into which the matching record (if found) is copied.
    attrType   → 'i', 'f', or 's' indicating the attribute type.
    attrSize   → attribute length in bytes.
    attrOffset → byte offset of attribute inside record.
    valuePtr   → pointer to constant being compared against.
    compOp     → comparison operator: CMP_EQ, CMP_NE, CMP_LT, CMP_LTE, CMP_GT, CMP_GTE.

FUNCTION DESCRIPTION:
    This routine implements the predicate-based scan underlying SELECT and DELETE operations.
    Starting just after startRid, every occupied slot in every subsequent page is examined. 
    For each record:
        1) The page is loaded (ReadPage).
        2) The slotmap is checked to ensure the slot is active.
        3) The record is copied into a temporary buffer.
        4) The attribute at attrOffset is compared with the literal stored in valuePtr using compareRecs().
        5) If the predicate holds, the record is copied into recPtr, foundRid is set, and OK is returned.
    If no matching record exists, OK is returned with foundRid = { -1, -1 }.
    Errors occur only if page access fails or memory cannot be allocated for a temporary record buffer.

RETURNS:
    OK     → success (record matched OR scan exhausted).
    NOTOK  → error (db_err_code set appropriately).

ALGORITHM:
    1) rid = IncRid(startRid, recsPerPg).
    2) Set *foundRid = { -1, -1 }.
    3) Clear recPtr to eliminate stale data.
    4) While rid.pid < numPgs:
        a) Call ReadPage(relNum, rid.pid).
            If NOTOK → return NOTOK.
        b) Read slotmap from page.
        c) If slot is occupied:
            - Allocate buffer curRec = malloc(recSize).
            - Copy record into curRec.
            - If compareRecs(curRec, valuePtr, …, compOp)
                then:
                    *foundRid = rid;
                    copy curRec → recPtr;
                    free curRec;
                    return OK;
            - free(curRec).
        d) rid = IncRid(rid, recsPerPg).
    5) No match found → return OK.

GLOBAL VARIABLES MODIFIED:
       None.

BUGS:
    None found.

ERRORS REPORTED:
    MEM_ALLOC_ERROR     – failure to allocate temporary record.

IMPLEMENTATION NOTES:
    - The caller interprets “no match” by checking foundRid->pid < 0.
    - compareRecs() handles integer, float (with NaN support), and fixed-length string comparisons.
    - This routine does NOT skip deleted pages; it only checks bit-level slot occupancy.

--------------------------------------------------------------*/

int FindRec(int relNum, Rid startRid, Rid *foundRid, void *recPtr, char attrType, int attrSize, int attrOffset, void *valuePtr, int compOp)
{
    int numPgs = catcache[relNum].relcat_rec.numPgs;
    int recsPerPg = catcache[relNum].relcat_rec.recsPerPg;
    int recSize = catcache[relNum].relcat_rec.recLength;
    char *page = buffer[relNum].page;
    
    Rid rid = IncRid(startRid, recsPerPg);
    *foundRid = (Rid){-1, -1};
    memset(recPtr, 0, recSize);

    while(rid.pid < numPgs)
    {
        if(ReadPage(relNum, rid.pid) == NOTOK)
        return NOTOK;

        unsigned long slotmap;
        memcpy(&slotmap, page+MAGIC_SIZE, sizeof(slotmap));

        if(slotmap & (1UL << rid.slotnum))
        {
            void *curRec = malloc(recSize);

            if(!curRec)
            {
                db_err_code = MEM_ALLOC_ERROR;
                return NOTOK;
            }

            memcpy(curRec, page+HEADER_SIZE+recSize*rid.slotnum, recSize);

            if(compareRecs(curRec, valuePtr, attrType, attrSize, attrOffset, compOp))
            {
                modifyRec(foundRid, rid, recPtr, curRec, recSize);
                return OK;
            }

            free(curRec);
        }

        rid = IncRid(rid, recsPerPg);
    }

    return OK;
}