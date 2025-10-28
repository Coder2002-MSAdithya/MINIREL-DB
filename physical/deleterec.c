#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "../include/defs.h"
#include "../include/globals.h"
#include "../include/error.h"
#include "../include/readpage.h"
#include "../include/writerec.h"

/* Read the given page of the relation and 
unset the slotmap at the slotnum position of the map */

int DeleteRec(int relNum, Rid recRid)
{
    CacheEntry *entry = &catcache[relNum];
    int numPgs = (entry->relcat_rec).numPgs;
    int recsPerPg = (entry->relcat_rec).recsPerPg;
    char *page = buffer[relNum].page;

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

    unsigned long slotmap;
    memcpy(&slotmap, page + MAGIC_SIZE, SLOTMAP);
    slotmap &= ~(1 << recRid.slotnum);
    memcpy(page + MAGIC_SIZE, &slotmap, SLOTMAP);
    buffer[relNum].dirty = true;

    catcache[relNum].relcat_rec.numRecs -= 1;

    if(!slotmap)
    {
        /* Can't do this here because the emptied page need NOT be the last one */
        // catcache[relNum].relcat_rec.numPgs -= 1;
    }

    catcache[relNum].status |= DIRTY_MASK;
    WriteRec(RELCAT_CACHE, &(entry->relcat_rec), entry->relcatRid);

    return OK;
}