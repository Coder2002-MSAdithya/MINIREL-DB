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
        return ErrorMsgs(INVALID_RELNUM, print_flag);
    }
        
    Buffer *buf = &buffer[relNum];
    CacheEntry *entry = &catcache[relNum];

    if (entry->relFile < 0)
    {
        return ErrorMsgs(REL_OPEN_ERROR, print_flag);
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
        return ErrorMsgs(FILESYSTEM_ERROR, print_flag);
    }

    ssize_t bytesRead = read(entry->relFile, buf->page, PAGESIZE);
    if (bytesRead != PAGESIZE)
    {
        return ErrorMsgs(FILESYSTEM_ERROR, print_flag);
    }

    // Check magic string (optional)
    Page *pg = (Page *)buf->page;
    if (strncmp(pg->magicString + 1, GEN_MAGIC, strlen(GEN_MAGIC)) != 0)
    {
        return ErrorMsgs(PAGE_MAGIC_ERROR, print_flag);
    }

    // Update buffer metadata
    buf->pid = pid;
    buf->relFile = entry->relFile;
    buf->dirty = 0;

    if (print_flag)
    {
        printf("[DEBUG] Read page %d of relation '%s' into buffer slot %d\n", pid, entry->relcat_rec.relName, relNum);
    }
    
    return OK;
}
