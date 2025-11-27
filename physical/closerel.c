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

FUNCTION CloseRel (argc, argv)

PARAMETER DESCRIPTION:
    relNum  →  Cache index of the relation to be closed.

FUNCTION DESCRIPTION:
    Closes an open relation represented in the catalog cache.
    If the relation’s catalog entry has been modified, the updated RelCatRec is written back to relcat. 
    If a data page in the buffer is dirty, it is flushed to disk.
    The routine then frees the attribute descriptor list, invalidates the cache entry, and closes the associated file descriptor.

ALGORITHM:
    1. Validate relNum → must be within bounds.
    2. Check if the cache entry is valid and currently open.
       If not valid, return NOTOK.
    3. If the catalog entry is marked dirty (DIRTY_MASK):
           Write back the modified RelCatRec into relcat using WriteRec().
    4. If the buffer page for this relation is dirty:
           Call FlushPage(relNum) to write page to disk.
    5. Clear VALID_MASK from entry->status to indicate closure.
    6. Free the attribute descriptor linked list using FreeLinkedList().
    7. Close the underlying file descriptor.
    8. Return OK.

ERRORS REPORTED:
    - Returns NOTOK on invalid relNum.
    - Returns NOTOK if entry is not open.

GLOBAL VARIABLES MODIFIED:
    - catcache[relNum].status (VALID bit cleared)
    - catcache[relNum].attrList (freed)
    - buffer[relNum] (dirty page flushed)
    - Potentially relcat via WriteRec()
    - file descriptors table (via close())

------------------------------------------------------------*/

int CloseRel(int relNum)
{
    if (relNum<0 || relNum>=MAXOPEN)
    {
        return NOTOK;
    }
        
    CacheEntry *entry = &catcache[relNum];

    if (!(entry->status & VALID_MASK))
    {
        return NOTOK;
    }

    if((entry->status) & DIRTY_MASK)
    {
        if(WriteRec(RELCAT_CACHE, &(entry->relcat_rec), entry->relcatRid) != OK)
        {
            return NOTOK;
        }
    }
        
    if(buffer[relNum].dirty)
    {
        FlushPage(relNum);
    }

    (entry->status) &= ~VALID_MASK;

    FreeLinkedList((void **)&(entry->attrList), offsetof(AttrDesc, next));
    close(entry->relFile);
    entry->relFile = -1;

    return OK;
}