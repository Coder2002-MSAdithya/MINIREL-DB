#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"


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
    buf->dirty = 0;

    if (true)
    {
        printf("[DEBUG] Read page %d of relation '%s' into buffer slot %d\n", pid, entry->relcat_rec.relName, relNum);
    }
    
    return OK;
}
