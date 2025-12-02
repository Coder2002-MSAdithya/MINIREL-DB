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

#define INS_NO_FREE_SLOT  2  /* internal code: page has no free slot */

/*
 * insertIntoPage:
 *   Try to insert recPtr into page pidx of relation relNum.
 *
 *   Parameters:
 *     relNum       - relation number
 *     pidx         - page index
 *     recPtr       - pointer to record data
 *     becameFull   - (out) set to true if page was not full but becomes full
 *     hasFreeAfter - (out) set to true if page has at least one free slot after insert
 *
 *   Returns:
 *     OK              -> inserted successfully
 *     INS_NO_FREE_SLOT-> page had no free slots
 *     NOTOK           -> error (db_err_code set)
 *
 *   This function ONLY:
 *     - works within buffer[relNum].page
 *     - updates slotmap, page contents
 *     - updates relcat_rec.numRecs and writes relcat
 *   It does NOT touch freemap; the caller handles that.
 */
static int insertIntoPage(int relNum,
                          short pidx,
                          void *recPtr,
                          bool *becameFull,
                          bool *hasFreeAfter)
{
    CacheEntry *entry = &catcache[relNum];
    char *page        = buffer[relNum].page;

    int recSize   = entry->relcat_rec.recLength;
    int recsPerPg = entry->relcat_rec.recsPerPg;

    if(ReadPage(relNum, pidx) == NOTOK)
    {
        return NOTOK;
    }

    /* Check magic */
    if (strncmp(page + 1, GEN_MAGIC, MAGIC_SIZE - 1))
    {
        db_err_code = PAGE_MAGIC_ERROR;
        return NOTOK;
    }

    unsigned long slotmap = 0;
    memcpy(&slotmap, page + MAGIC_SIZE, SLOTMAP);

    /* Compute mask of all valid slots */
    unsigned long fullMask;
    if (recsPerPg >= (int)(8 * sizeof(unsigned long)))
        fullMask = ~0UL;
    else
        fullMask = (1UL << recsPerPg) - 1;

    bool wasFull = ((slotmap & fullMask) == fullMask);

    /* Find a free slot */
    for (short slot = 0; slot < recsPerPg; slot++)
    {
        if (!(slotmap & (1UL << slot)))  /* free slot */
        {
            entry->relcat_rec.numRecs += 1;
            entry->status |= DIRTY_MASK;

            if(WriteRec(RELCAT_CACHE, &(entry->relcat_rec), entry->relcatRid) == NOTOK)
            {
                entry->relcat_rec.numRecs -= 1;
                return NOTOK;
            }

            int offset = HEADER_SIZE + slot * recSize;
            memcpy(page + offset, recPtr, recSize);

            slotmap |= (1UL << slot);
            memcpy(page + MAGIC_SIZE, &slotmap, SLOTMAP);
            buffer[relNum].dirty = true;

            if (becameFull)
                *becameFull = (!wasFull && ((slotmap & fullMask) == fullMask));
            if (hasFreeAfter)
                *hasFreeAfter = ((slotmap & fullMask) != fullMask);

            return OK;
        }
    }

    /* No free slot in this page */
    if (becameFull)
        *becameFull = false;
    if (hasFreeAfter)
        *hasFreeAfter = false;
    return INS_NO_FREE_SLOT;
}

int InsertRec(int relNum, void *recPtr)
{
    CacheEntry *entry = &catcache[relNum];

    if (!(entry->status & VALID_MASK))
    {
        db_err_code = INVALID_RELNUM;
        return NOTOK;
    }

    int recsPerPg = entry->relcat_rec.recsPerPg;
    int numPages  = entry->relcat_rec.numPgs;
    char *page    = buffer[relNum].page;
    const char *relName = entry->relcat_rec.relName;

    /* Whether to use freemap for this relation */
    bool useFreeMap = (FreeMapExists(relName) == 1);

    /* -------- 1. Try freemap first if available -------- */
    if (useFreeMap)
    {
        int freePg = FindFreeSlot(relName);

        if (freePg >= 0 && freePg < numPages)
        {
            short pidx = (short)freePg;
            bool becameFull = false;
            bool hasFreeAfter = false;

            int rc = insertIntoPage(relNum, pidx, recPtr,
                                    &becameFull, &hasFreeAfter);

            if (rc == OK)
            {
                if (becameFull)
                {
                    /* Page just became full: remove from freemap */
                    DeleteFromFreeMap(relName, pidx);
                }
                else if (hasFreeAfter)
                {
                    /* Page still has space: ensure in freemap */
                    AddToFreeMap(relName, pidx);
                }
                return OK;
            }

            if (rc == NOTOK)
                return NOTOK;

            /* rc == INS_NO_FREE_SLOT:
               freemap was stale for this page -> clean it up and fall back */
            DeleteFromFreeMap(relName, pidx);
        }
    }

    /* -------- 2. Fallback: linear scan over all existing pages -------- */
    for (short pidx = 0; pidx < numPages; pidx++)
    {
        bool becameFull = false;
        bool hasFreeAfter = false;

        int rc = insertIntoPage(relNum, pidx, recPtr,
                                &becameFull, &hasFreeAfter);

        if (rc == OK)
        {
            if (useFreeMap)
            {
                if (becameFull)
                {
                    DeleteFromFreeMap(relName, pidx);
                }
                else if (hasFreeAfter)
                {
                    AddToFreeMap(relName, pidx);
                }
            }
            return OK;
        }

        if (rc == NOTOK)
            return NOTOK;

        /* rc == INS_NO_FREE_SLOT: just try next page */
    }

    /* -------- 3. No free slot: allocate a new page -------- */
    if(FlushPage(relNum) == NOTOK)
    {
        return NOTOK;
    }

    buffer[relNum].pid   = numPages;
    buffer[relNum].dirty = true;
    memset(page, 0, PAGESIZE);

    if(buffer[relNum].pid < 0)
    {
        db_err_code = REL_PAGE_LIMIT_REACHED;
        return NOTOK;
    }

    char c = (relNum == 0 ? '$' : (relNum == 1 ? '!' : '_'));
    unsigned long newMap = 1UL;  /* occupy slot 0 */

    page[0] = c;
    strncpy(page + 1, GEN_MAGIC, MAGIC_SIZE - 1);
    memcpy(page + MAGIC_SIZE, &newMap, SLOTMAP);

    int recSize = entry->relcat_rec.recLength;
    memcpy(page + HEADER_SIZE, recPtr, recSize);

    entry->relcat_rec.numRecs += 1;
    entry->relcat_rec.numPgs  += 1;
    entry->status |= DIRTY_MASK;

    if(WriteRec(RELCAT_CACHE, &(entry->relcat_rec), entry->relcatRid) == NOTOK)
    {
        entry->relcat_rec.numRecs -= 1;
        entry->relcat_rec.numPgs  -= 1;
        return NOTOK;
    }

    /* New page has free slots if recsPerPg > 1 */
    if (useFreeMap && recsPerPg > 1)
        AddToFreeMap(relName, (short)numPages);

    return OK;
}