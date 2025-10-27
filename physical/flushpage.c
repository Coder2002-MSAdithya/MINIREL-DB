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
        return ErrorMsgs(INVALID_RELNUM, print_flag);
    }

    Buffer *buf = &buffer[relNum];
    CacheEntry *entry = &catcache[relNum];

    if (entry->relFile < 0)
    {
        return ErrorMsgs(REL_OPEN_ERROR, print_flag);
    }

    // Nothing to flush
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