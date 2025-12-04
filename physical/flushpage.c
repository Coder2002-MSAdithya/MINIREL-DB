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
    relNum → index into the catcache[]

FUNCTION DESCRIPtION:
    The routine writes the dirty page currently held in buffer[relNum] back to disk. 
    If no dirty bit is set, the routine returns immediately. 
    If the relation file descriptor is invalid, or if any filesystem operation fails, an error is reported.

ALGORITHM:   
    1) Validates relNum.
    2) Checks whether the relation's file descriptor is valid.
    3) Returns OK immediately if the page is not dirty.
    4) Computes the correct byte offset = pid * PAGESIZE.
    5) Seeks to the offset and writes PAGESIZE bytes.
    6) Clears the dirty flag upon successful write.

BUGS:
    None found.

ERRORS REPORTED:
    INVALID_RELNUM
    FILESYSTEM_ERROR

GLOBAL VARIABLES MODIFIED:
    buffer[relNum].dirty
    db_err_code (on errors)

IMPLEMENTATION NOTES:
    • FlushPage() is invoked automatically by ReadPage() when switching to a new page, and by CloseRel() when closing the relation.

------------------------------------------------------------*/

int FlushPage(int relNum)
{
    if (relNum < 0 || relNum >= MAXOPEN)
    {
        db_err_code = NOTOK;
        return NOTOK;
    }

    Buffer *buf = &buffer[relNum];
    CacheEntry *entry = &catcache[relNum];

    if (entry->relFile < 0)
    {
        db_err_code = FILESYSTEM_ERROR;
        return NOTOK;
    }

    // Nothing to flush
    if (buf->dirty == 0)
    {
        return OK;
    }

    off_t offset = (off_t)buf->pid * PAGESIZE;

    if (lseek(entry->relFile, offset, SEEK_SET) < 0)
    {
        db_err_code = FILESYSTEM_ERROR;
        return NOTOK;
    }

    ssize_t bytesWritten = write(entry->relFile, buf->page, PAGESIZE);
    if (bytesWritten != PAGESIZE)
    {
        db_err_code = FILESYSTEM_ERROR;
        return NOTOK;
    }

    buf->dirty = 0;

    return OK;
}