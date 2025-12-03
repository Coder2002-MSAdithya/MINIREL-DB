#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"

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