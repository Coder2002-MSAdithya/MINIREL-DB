#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../include/defs.h"
#include "../include/helpers.h"
#include "../include/globals.h"
#include "../include/readpage.h"
#include "../include/error.h"

void modifyRec(Rid *foundRid, Rid rid, void *recPtr, void *curRec, int recSize)
{
    *foundRid = rid;
    memcpy(recPtr, curRec, recSize);
}

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