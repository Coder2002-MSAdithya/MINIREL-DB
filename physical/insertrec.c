#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "../include/defs.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/error.h"
#include "../include/readpage.h"
#include "../include/writerec.h"
#include "../include/freemap.h"

#define INS_NO_FREE_SLOT  2  /* internal code: page has no free slot */


/*------------------------------------------------------------

FUNCTION insertIntoPage (relNum, pidx, recPtr, becameFull, hasFreeAfter)

PARAMETER DESCRIPTION:
    relNum       → relation number in the open-relation cache.
    pidx         → page index within the relation.
    recPtr       → pointer to the record to be inserted.
    becameFull   → set to TRUE iff the page was not previously full but becomes full after insert.
    hasFreeAfter → set to TRUE iff, after insertion, the page still contains at least one free slot.

FUNCTION DESCRIPTION:
    This routine attempts to insert a single record into a specific page of a relation. 
    It performs no relation-level scanning; it works strictly on the already-known target page.
    It updates:
        - page slotmap,
        - page contents,
        - the relation’s numRecs field in relcat.
    It does not update the freemap. The caller must handle freemap maintenance.

RETURNS:
       OK               → record successfully inserted.
       INS_NO_FREE_SLOT → page contains no empty slot.
       NOTOK            → error encountered (db_err_code set).

ALGORITHM:
    1) Read the target page into the buffer using ReadPage().
    2) Validate page magic bytes.
    3) Load the slotmap and compute the mask of valid bits.
    4) Identify whether the page was previously full.
    5) Scan for the first free slot.
    6) When found:
        a) Increment numRecs for the relation.
        b) Update the relcat entry via WriteRec().
        c) Copy the new record into the slot’s data region.
        d) Set the bit in the slotmap; mark buffer page dirty.
        e) Set becameFull and hasFreeAfter as appropriate.
        f) Return OK.
    7) If no free slot, return INS_NO_FREE_SLOT.

GLOBAL VARIABLES MODIFIED:
    catcache[relNum].relcat_rec.numRecs
    catcache[relNum].status 
    buffer[relNum].page
    buffer[relNum].dirty
    db_err_code on failure.

ERRORS REPORTED:
    PAGE_MAGIC_ERROR

IMPLEMENTATION NOTES:
    This function assumes the caller has already determined that the page is a valid page for insertion. 
    No freemap updates occur inside this helper.

--------------------------------------------------------------*/

static int insertIntoPage(int relNum, short pidx, void *recPtr, bool *becameFull, bool *hasFreeAfter)
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
    if (recsPerPg >= (int)(sizeof(unsigned long) << 3))
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
            
            int offset = HEADER_SIZE + slot * recSize;
            memcpy(page + offset, recPtr, recSize);
            
            slotmap |= (1UL << slot);
            memcpy(page + MAGIC_SIZE, &slotmap, SLOTMAP);
            buffer[relNum].dirty = true;

            if(WriteRec(RELCAT_CACHE, &(entry->relcat_rec), entry->relcatRid) == NOTOK)
            {
                entry->relcat_rec.numRecs -= 1;
                return NOTOK;
            }

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


/*------------------------------------------------------------

FUNCTION InsertRec (relNum, recPtr)

PARAMETER DESCRIPTION:
    relNum → index of the open relation in catcache.
    recPtr → pointer to the record to be inserted.

FUNCTION DESCRIPTION:
    Implements the core file insertion routine.
    Attempts to place the incoming tuple in:
        1) a free page identified via freemap (if available),
        2) otherwise a linear scan of all existing pages,
        3) otherwise a newly allocated page.
    Works with fixed-length records. Maintains:
        - relation page count,
        - tuple count,
        - slotmaps,
        - page flushes,
        - freemap bitmap.

RETURNS:
    OK    → record inserted successfully.
    NOTOK → error (db_err_code set appropriately).

ALGORITHM:
    1) Validate relation is open and entry is marked VALID_MASK.
    2) Retrieve relation metadata: recsPerPg, numPages, relation name, etc.
    3) If freemap file available for the relation.
        a) Obtain a page number from FindFreeSlot().
        b) Attempt insertion into that page via insertIntoPage().
        c) If successful:
            - update freemap: remove page if it became full, or ensure page remains marked free if still not full.
            - return OK.
        d) If page was stale (INS_NO_FREE_SLOT), clear it in freemap.
    4) Else scan all existing pages sequentially.
        - For each page pidx: call insertIntoPage().
        - If OK: update freemap accordingly, return OK.
    5) If all pages full, allocate a new page:
        a) Flush current buffer page (FlushPage()).
        b) Initialize a new empty page with: page-type marker, magic bytes, slotmap containing only slot 0 occupied.
        c) Copy record into slot 0.
        d) Update relation metadata: numRecs++, numPgs++, WriteRec() the relcat entry.
        e) If using freemap and recsPerPg > 1: Add the new page to freemap.
    6) Return OK.

BUGS:
    None found.

GLOBAL VARIABLES MODIFIED:
    catcache[relNum].relcat_rec.numRecs
    catcache[relNum].relcat_rec.numPgs
    catcache[relNum].status (DIRTY_MASK)
    buffer[relNum].page
    buffer[relNum].pid
    buffer[relNum].dirty

ERRORS REPORTED:
    INVALID_RELNUM
    PAGE_MAGIC_ERROR
    FILESYSTEM_ERROR
    REL_PAGE_LIMIT_REACHED
    REL_OPEN_ERROR

--------------------------------------------------------------*/

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