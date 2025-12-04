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

FUNCTION ReadPage (relNum, pid)

PARAMETER DESCRIPTION:
    relNum → integer index into the catcache[]
    pid    → page identifier (0 ≤ pid < number of pages in relation)

FUNCTION DESCRIPTION:
    This routine loads page pid of relation relNum into that relation’s buffer slot (buffer[relNum]). 
    If the page already resides in the buffer, no disk access is performed.
    If the current page in the buffer is dirty, it is first flushed to disk via FlushPage(). 
    After ensuring consistency, ReadPage positions the file pointer to the correct location and reads exactly PAGESIZE bytes into buffer[relNum].page.

ALGORITHM:
    1) Validate relNum and pid against legal bounds.
    2) Verify that the relation file descriptor is valid.
    3) If the requested page is already buffered, return OK.
    4) If the buffer contains a dirty page, flush it.
    5) Compute the byte offset pid*PAGESIZE.
    6) Seek to the offset and read PAGESIZE bytes into memory.
    7) Update buffer metadata (pid, dirty flag).
    8) Return OK upon success, NOTOK otherwise.

BUGS:
    None found.

ERRORS REPORTED:
    INVALID_RELNUM
    PAGE_OUT_OF_BOUNDS
    REL_OPEN_ERROR
    FILESYSTEM_ERROR

GLOBAL VARIABLES MODIFIED:
    buffer[relNum].pid
    buffer[relNum].dirty
    db_err_code (on errors)

------------------------------------------------------------*/

int ReadPage(int relNum, short pid)
{
    // Validate relation number
    if (relNum < 0 || relNum >= MAXOPEN)
    {
        db_err_code = INVALID_RELNUM;
        return NOTOK;
    }

    Buffer *buf = &buffer[relNum];
    CacheEntry *entry = &catcache[relNum];

    if(pid < 0 || pid >= (entry->relcat_rec).numPgs)
    {
        db_err_code = PAGE_OUT_OF_BOUNDS;
        return NOTOK;
    }
    
    if (entry->relFile < 0)
    {
        db_err_code = REL_OPEN_ERROR;
        return NOTOK;
    }

    // Already loaded
    if (buf->pid == pid)
    {
        return OK;
    }

    // If dirty, flush current page
    if (buf->dirty)
    {
        int res = FlushPage(relNum);
        if (res != OK)
            return res;
    }

    // Compute byte offset
    off_t offset = (off_t)pid * PAGESIZE;

    // Seek and read from file
    if (lseek(entry->relFile, offset, SEEK_SET) < 0)
    {
        db_err_code = FILESYSTEM_ERROR;
        return NOTOK;
    }

    ssize_t bytesRead = read(entry->relFile, buf->page, PAGESIZE);
    if (bytesRead != PAGESIZE)
    {
        db_err_code = FILESYSTEM_ERROR;
        return NOTOK;
    }

    // Update buffer metadata
    buf->pid = pid;
    buf->dirty = false;
    
    return OK;
}
