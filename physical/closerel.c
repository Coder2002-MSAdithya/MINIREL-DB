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

    // Step 4: Close file
    close(entry->relFile);

    return OK;
}