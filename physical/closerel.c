/************************INCLUDES*******************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/writerec.h"


/*------------------------------------------------------------

FUNCTION CloseRel (relNum)

PARAMETER DESCRIPTION:
    relNum  → integer index identifying a relation’s slot inside the open-relation cache (catcache[]).

FUNCTION DESCRIPTION:
    Closes an open relation whose metadata and file are maintained in catcache[relNum] and buffer[relNum].
    The routine must:
        - Write catalog metadata back to relcat if dirty.
        - Flush any dirty page belonging to this relation.
        - Free the attribute-descriptor linked list.
        - Close the underlying file descriptor.
        - Mark the cache slot as invalid for future reuse.
    If the relation is not currently open, the routine performs no operation and returns OK.

ALGORITHM:
    1) Validate relNum boundaries.
    2) If the cache slot is not valid, return OK immediately. (Caller may safely call CloseRel on unopened slots.)
    3) If the catalog metadata in catcache[relNum] has been modified (DIRTY_MASK set), write updated RelCatRec to relcat using WriteRec().
    4) Check whether the buffer page for this relation is dirty; if yes, write it to disk using FlushPage().
    5) Free the linked list of AttrDesc nodes via FreeLinkedList(), disconnecting catalog attribute metadata associated with this relation.
    6) Close the file descriptor for this relation.
    7) Clear VALID_MASK in status, marking the slot free.

BUGS:
    None found.

ERRORS REPORTED:
    INVALID_RELNUM       – if relNum outside valid slot range

GLOBAL VARIABLES MODIFIED:
    catcache[relNum]     – status flags, metadata, attrList freed
    buffer[relNum]       – dirty flag cleared after flushing
    db_err_code          – updated on error

------------------------------------------------------------*/

int CloseRel(int relNum)
{
    if (relNum < 0 || relNum >= MAXOPEN)
        return NOTOK;

    CacheEntry *entry = &catcache[relNum];

    if (!(entry->status & VALID_MASK))
        return OK; // Not open, so nothing to do

    // Step 1: If catalog info dirty, update relcat in the buffer
    if((entry->status) & DIRTY_MASK)
    {
        if(WriteRec(RELCAT_CACHE, &(entry->relcat_rec), entry->relcatRid) == NOTOK)
        {
            return NOTOK;
        }
    }
        
    // Step 2: Flush dirty page if any
    if(buffer[relNum].dirty)
    {
        if(FlushPage(relNum) == NOTOK)
        {
            return NOTOK;
        }
    }

    //Step 3: Invalidate cache entry
    (entry->status) &= ~VALID_MASK;

    //Step 4: Free the linked list of attribute descriptors
    FreeLinkedList((void **)&(entry->attrList), offsetof(AttrDesc, next));

    // Step 5: Close file
    close(entry->relFile);

    //Step 6: Invalidate the buffer pool of the relation
    buffer[relNum].pid = -1;

    return OK;
}