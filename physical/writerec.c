/************************INCLUDES*******************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "../include/globals.h"
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/readpage.h"


/*------------------------------------------------------------

FUNCTION WriteRec (relNum, recPtr, recRid)

PARAMETER DESCRIPTION:
    relNum  → relation number (index into catcache[]).
    recPtr  → pointer to the new record contents to be written.
    recRid  → identifies the record to overwrite (page id = pid, slot number = slotnum).

FUNCTION DESCRIPTION:
    This routine overwrites an existing record in a relation.
    It:
        - validates the relation id and record location,
        - loads the target page into the buffer (ReadPage),
        - copies the new tuple into the correct slot,
        - marks the buffer page dirty.
    It does NOT:
        - update catalogs,
        - update slotmap,

RETURNS:
    OK     → record overwritten successfully.
    NOTOK  → error occurred (db_err_code set appropriately).

ALGORITHM:
    1) Validate that relNum corresponds to a valid open relation.
    2) Validate recRid:
        - pid ≥ 0, slotnum ≥ 0
        - pid < number of pages
        - slotnum < records per page
        If invalid, set PAGE_OUT_OF_BOUNDS and return NOTOK.
    3) Call ReadPage(relNum, recRid.pid) to load the page.
    4) Compute the memory location of the target slot:
            offset = HEADER_SIZE + slotnum * recSize.
    5) Overwrite the record using memcpy().
    6) Mark the buffer for this relation as dirty.
    7) Return OK.

BUGS:
    None found.

GLOBAL VARIABLES MODIFIED:
    buffer[relNum].page
    buffer[relNum].dirty
    db_err_code (on failure)

ERRORS REPORTED:
    INVALID_RELNUM     – relation is not open.
    PAGE_OUT_OF_BOUNDS – pid or slotnum outside valid range.

--------------------------------------------------------------*/

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

    if(ReadPage(relNum, recRid.pid) == NOTOK)
    {
        return NOTOK;
    }

    void *recToUpdate = page + HEADER_SIZE + recRid.slotnum * recSize;
    memcpy(recToUpdate, recPtr, recSize);
    buffer[relNum].dirty = true;

    return OK;
}
