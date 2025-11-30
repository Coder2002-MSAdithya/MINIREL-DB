/************************INCLUDES*******************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"


/*------------------------------------------------------------

FUNCTION FlushPage (relNum)

PARAMETER DESCRIPTION:
    relNum → Cache slot index of an open relation. 
             Must be in the range 0 ≤ relNum < MAXOPEN. 
             The corresponding cache entry must represent a valid, open relation whose buffer may or may not be dirty.

FUNCTION DESCRIPTION:
    Writes the currently loaded page from the buffer slot associated with relNum back to disk if the page is marked dirty. 
    If the page is not dirty, no action is taken. 
    After a successful write, the dirty flag is cleared.

ALGORITHM:
    1. Validate relNum.
    2. Retrieve cache entry and buffer entry for relNum.
    3. If the underlying file descriptor is invalid, report REL_OPEN_ERROR.
    4. If the buffer is not dirty, return OK.
    5. Compute byte offset = pid * PAGESIZE.
    6. Seek to that offset in the relation file.
    7. Write PAGESIZE bytes from the buffer to disk.
    8. If the write succeeds, set dirty = 0.
    9. Return OK.

ERRORS REPORTED:
    INVALID_RELNUM     → If relNum is outside valid bounds.
    REL_OPEN_ERROR     → If relation file descriptor is invalid.
    FILESYSTEM_ERROR   → If lseek or write fails.

GLOBAL VARIABLES MODIFIED:
    - buffer[relNum].dirty

IMPLEMENTATION NOTES:
    - This function is part of MiniRel’s write-back policy. The buffer may contain at most one page per open relation in this design.
    - Caller must ensure that the correct page is already loaded in the buffer before calling FlushPage.

------------------------------------------------------------*/

int FlushPage(int relNum)
{
    if (relNum<0 || relNum>=MAXOPEN)
    {
        return ErrorMsgs(INVALID_RELNUM, print_flag);
    }

    Buffer *buf = &buffer[relNum];
    CacheEntry *entry = &catcache[relNum];

    if (entry->relFile < 0)
    {
        return ErrorMsgs(REL_OPEN_ERROR, print_flag);
    }

    if (buf->dirty == 0)
    {
        return OK;
    }

    off_t offset = (off_t)buf->pid * PAGESIZE;

    if (lseek(entry->relFile, offset, SEEK_SET) < 0)
    {
        return ErrorMsgs(FILESYSTEM_ERROR, print_flag);
    }

    ssize_t bytesWritten = write(entry->relFile, buf->page, PAGESIZE);
    if (bytesWritten != PAGESIZE)
    {
        return ErrorMsgs(FILESYSTEM_ERROR, print_flag);
    }

    buf->dirty = 0;

    if (debug_flag)
    {
        // printf("[DEBUG] Flushed dirty page %d of relation '%s' to disk\n", buf->pid, entry->relcat_rec.relName);
    }

    return OK;
}