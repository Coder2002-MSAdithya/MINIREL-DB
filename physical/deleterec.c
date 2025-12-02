#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "../include/defs.h"
#include "../include/globals.h"
#include "../include/error.h"
#include "../include/readpage.h"
#include "../include/writerec.h"
#include "../include/freemap.h"

/*
 * deleteFromPage:
 *   Clear the slot bit for recRid.slotnum on page recRid.pid of relNum.
 *
 *   Parameters:
 *     relNum         - relation number
 *     recRid         - page id + slot number
 *     wasFullBefore  - (out) true if page was full before deletion
 *     hasFreeAfter   - (out) true if page has at least one free slot after deletion
 *
 *   This function:
 *     - reads the page into buffer[relNum].page
 *     - updates the slotmap (clears a bit)
 *     - marks buffer dirty
 *     - decrements relcat_rec.numRecs and writes relcat
 *
 *   It does NOT touch the freemap; the caller handles that.
 */
static int deleteFromPage(int relNum,
                          Rid recRid,
                          bool *wasFullBefore,
                          bool *hasFreeAfter)
{
    CacheEntry *entry = &catcache[relNum];
    char *page        = buffer[relNum].page;

    int recsPerPg = entry->relcat_rec.recsPerPg;

    if (ReadPage(relNum, recRid.pid) == NOTOK)
    {
        db_err_code = REL_OPEN_ERROR;
        return NOTOK;
    }

    unsigned long slotmap = 0;
    memcpy(&slotmap, page + MAGIC_SIZE, SLOTMAP);
    unsigned long oldSlotmap = slotmap;

    /* Compute mask of all valid slots */
    unsigned long fullMask;
    if (recsPerPg >= (int)(8 * sizeof(unsigned long)))
        fullMask = ~0UL;
    else
        fullMask = (1UL << recsPerPg) - 1;

    if (wasFullBefore)
        *wasFullBefore = ((oldSlotmap & fullMask) == fullMask);

    /* Clear the bit for this slot */
    slotmap &= ~(1UL << recRid.slotnum);
    memcpy(page + MAGIC_SIZE, &slotmap, SLOTMAP);
    buffer[relNum].dirty = true;

    entry->relcat_rec.numRecs -= 1;
    entry->status |= DIRTY_MASK;
    if(WriteRec(RELCAT_CACHE, &(entry->relcat_rec), entry->relcatRid) == NOTOK)
    {
        entry->relcat_rec.numRecs += 1;
        return NOTOK;
    }

    /* After deletion, page has free slot iff it is NOT full */
    if (hasFreeAfter)
        *hasFreeAfter = ((slotmap & fullMask) != fullMask);

    return OK;
}

/* Public DeleteRec: does validation + freemap updates */
int DeleteRec(int relNum, Rid recRid)
{
    CacheEntry *entry = &catcache[relNum];
    int numPgs    = entry->relcat_rec.numPgs;
    int recsPerPg = entry->relcat_rec.recsPerPg;
    const char *relName = entry->relcat_rec.relName;

    if (recRid.pid < 0 || recRid.slotnum < 0)
    {
        db_err_code = PAGE_OUT_OF_BOUNDS;
        return NOTOK;
    }

    if (recRid.pid >= numPgs || recRid.slotnum >= recsPerPg)
    {
        db_err_code = PAGE_OUT_OF_BOUNDS;
        return NOTOK;
    }

    bool wasFullBefore = false;
    bool hasFreeAfter  = false;

    int rc = deleteFromPage(relNum, recRid, &wasFullBefore, &hasFreeAfter);
    if (rc == NOTOK)
        return NOTOK;

    /* Freemap maintenance (if it exists) */
    bool useFreeMap = (FreeMapExists(relName) == 1);

    if (useFreeMap)
    {
        /* Only if page was full before AND now has a free slot,
           do we need to add it to the freemap. */
        if (wasFullBefore && hasFreeAfter)
        {
            AddToFreeMap(relName, (short)recRid.pid);
        }
    }

    return OK;
}