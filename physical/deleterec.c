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

/*------------------------------------------------------------

FUNCTION deleteFromPage (relNum, recRid, wasFullBefore, hasFreeAfter)

PARAMETER DESCRIPTION:
    relNum         → open-relation number in catcache.
    recRid         → identifies the record to delete (page id + slot number).
    wasFullBefore  → set TRUE if the page was completely full before deletion.
    hasFreeAfter   → set TRUE if after deletion the page has at least one available slot.

FUNCTION DESCRIPTION:
    This routine deletes a tuple from a single page of a relation.
    It:
        - loads the page into the buffer,
        - clears the corresponding bit in the slotmap,
        - marks the page dirty,
        - decrements the tuple count in relcat,
        - writes back the relcat update.
    It DOES NOT update the freemap bitmap.  
    Freemap maintenance must be done by DeleteRec().

RETURNS:
    OK    → Deletion successful.
    NOTOK → Error encountered (db_err_code set).

ALGORITHM:
    1) Call ReadPage() to bring page recRid.pid into buffer.
    2) Retrieve slotmap from the page.
    3) Compute mask of valid slots and check if the page was full.
    4) Clear the bit corresponding to recRid.slotnum.
    5) Overwrite the page’s slotmap; mark page dirty.
    6) Decrement numRecs in relcat_rec; mark catalog dirty.
    7) Write updated relcat entry via WriteRec().
    8) Determine hasFreeAfter by checking whether page is no longer full.
    9) Return OK.

GLOBAL VARIABLES MODIFIED:
    buffer[relNum].page
    buffer[relNum].dirty
    catcache[relNum].relcat_rec.numRecs
    catcache[relNum].status (DIRTY_MASK)
    db_err_code on failure.

ERRORS REPORTED:
    REL_OPEN_ERROR

IMPLEMENTATION NOTES:
    - Page contents other than slotmap are not zeroed; the record bytes remain but are considered logically deleted.
    - Caller must update freemap based on wasFullBefore and hasFreeAfter.

--------------------------------------------------------------*/

static int deleteFromPage(int relNum, Rid recRid, bool *wasFullBefore, bool *hasFreeAfter)
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


/*------------------------------------------------------------

FUNCTION DeleteRec (relNum, recRid)

PARAMETER DESCRIPTION:
    relNum → open-relation number in catcache.
    recRid → page-slot identifier of the record to delete.

FUNCTION DESCRIPTION:
    Implements the logical deletion of a tuple from a relation.
    Performs validation, delegates page-level deletion to deleteFromPage(), and performs freemap maintenance.

RETURNS:
    OK    → Deletion successful.
    NOTOK → Error encountered (db_err_code set).

ALGORITHM:
    1) Validate recRid:
        - slotnum ≥ 0, pid ≥ 0
        - pid < numPgs, slotnum < recsPerPg
        Else report PAGE_OUT_OF_BOUNDS.
    2) Call deleteFromPage() to:
        - clear slot bit,
        - decrement numRecs,
        - update relcat,
        - return page state transitions.
    3) If freemap exists for this relation:
        If the page was full before and the page now has at least one free slot:
            → Add page to freemap (AddToFreeMap()).
    4) Return OK.

BUGS:
    None found.

GLOBAL VARIABLES MODIFIED:
    catcache[relNum].relcat_rec.numRecs
    catcache[relNum].status (DIRTY_MASK)
    buffer[relNum].page
    buffer[relNum].dirty
    db_err_code on error.

ERRORS REPORTED:
    PAGE_OUT_OF_BOUNDS
    FILESYSTEM_ERROR
    REL_OPEN_ERROR

--------------------------------------------------------------*/

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