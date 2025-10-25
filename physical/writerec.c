#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "../include/globals.h"
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/readpage.h"

int WriteRec(int relNum, void *recPtr, Rid recRid)
{
    int recSize = catcache[relNum].relcat_rec.recLength;
    int numPgs = catcache[relNum].relcat_rec.numPgs;
    int recsPerPg = catcache[relNum].relcat_rec.recsPerPg;
    char *page = buffer[relNum].page;

    if(!(catcache[relNum].status & VALID_MASK))
    {
        db_err_code = INVALID_RELNUM;
        return NOTOK;
    }

    if(recRid.pid < 0 || recRid.slotnum < 0)
    {
        db_err_code = PAGE_OUT_OF_BOUNDS;
        return NOTOK;
    }

    if(recRid.pid >= numPgs || recRid.slotnum >= recsPerPg)
    {
        db_err_code = PAGE_OUT_OF_BOUNDS;
        return NOTOK;
    }

    ReadPage(relNum, recRid.pid);

    void *recToUpdate = page + HEADER_SIZE + recRid.slotnum * recSize;
    memcpy(recToUpdate, recPtr, recSize);
    buffer[relNum].dirty = true;

    return OK;
}
