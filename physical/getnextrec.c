#include <stdio.h>
#include <string.h>
#include "../include/readpage.h"
#include "../include/defs.h"
#include "../include/globals.h"
#include "../include/error.h"
#include "../include/helpers.h"


/*--------------------------------------------------------------

FUNCTION GetNextRec (relNum, startRid, foundRid, recPtr)

PARAMETER DESCRIPTION:
    relNum    → relation number (index in catcache[]).
    startRid  → the RID from which the search should continue. 
    foundRid  → (OUT) receives the RID of the next valid record if one exists, otherwise { -1, -1 }.
    recPtr    → pointer to a buffer where the located record will be copied.

FUNCTION DESCRIPTION:
    This routine implements a sequential scan over a file.
    Given a starting RID, it returns the next occupied record (active slot) in physical order:
        page 0, slot 0 → page 0, slot 1 → … → page N.
    The function:
        - increments startRid using IncRid(),
        - loads each candidate page via ReadPage(),
        - checks the slotmap for an occupied slot,
        - copies the record into recPtr,
        - returns the RID via foundRid.
    If no further records exist, foundRid is set to {-1,-1} and OK is returned.
    Errors occur only if reading a page fails.

RETURNS:
    OK     → success (record found OR no more records).
    NOTOK  → an error occurred (db_err_code set).

ALGORITHM:
    1) Compute the next RID: rid = IncRid(startRid, recsPerPg).
    2) Initialize foundRid = { -1, -1 }.
    3) Zero-out recPtr to remove stale data.
    4) While rid.pid < numPgs:
        a) Read the page using ReadPage().
            If NOTOK → return NOTOK.
        b) Load slotmap.
        c) If slotmap indicates the slot is occupied:
            - copy the record into recPtr,
            - set *foundRid = rid,
            - return OK.
        d) Otherwise increment rid = IncRid(rid, recsPerPg).
    5) No more records: return OK.

BUGS:
    None found.

GLOBAL VARIABLES MODIFIED:
    None.

ERRORS REPORTED:
    None directly.

--------------------------------------------------------------*/

int GetNextRec(int relNum, Rid startRid, Rid *foundRid, void *recPtr)
{
    int numPgs = catcache[relNum].relcat_rec.numPgs;
    int numRecs = catcache[relNum].relcat_rec.numRecs;
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
            *foundRid = rid;
            memcpy(recPtr, page+HEADER_SIZE+recSize*rid.slotnum, recSize);
            return OK;
        }

        rid = IncRid(rid, recsPerPg);
    }

    return OK;
}
