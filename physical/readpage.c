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
    relNum → Cache slot number of an open relation. 
             Must satisfy 0 ≤ relNum < MAXOPEN and the cache entry must be VALID. 
    pid    → Page identifier within the relation file.
             Valid values: 0 ≤ pid < number of pages for this relation.

FUNCTION DESCRIPTION:
    Loads the specified page (pid) of the given relation into the corresponding buffer slot. 
    If a different page is already present in that buffer slot and is dirty, it is written out before loading the new page.

ALGORITHM:
    1. Validate relNum; ensure cache entry is VALID and file is open.
    2. Validate pid using relation's catalog record.
    3. If the requested page is already in buffer, return OK.
    4. If a different page is in the buffer and is marked dirty, call FlushPage() to write it out.
    5. Compute byte offset = pid * PAGESIZE.
    6. Perform lseek() to the target offset.
    7. Read PAGESIZE bytes into buffer[relNum].page.
    8. Update buffer metadata:
        - buffer[relNum].pid   = pid
        - buffer[relNum].dirty = false
    9. Return OK.

ERRORS REPORTED:
    INVALID_RELNUM       → relNum out of bounds.
    PAGE_OUT_OF_BOUNDS   → pid ≥ number of pages for this relation.
    REL_OPEN_ERROR       → underlying file descriptor is invalid.
    FILESYSTEM_ERROR     → lseek or read failed.

GLOBAL VARIABLES MODIFIED:
    - buffer[relNum].page
    - buffer[relNum].pid
    - buffer[relNum].dirty
    - db_err_code

------------------------------------------------------------*/

int ReadPage(int relNum, short pid)
{
    if(relNum<0 || relNum>=MAXOPEN)
    {
        db_err_code = INVALID_RELNUM;
        return NOTOK;
    }

    Buffer *buf = &buffer[relNum];
    CacheEntry *entry = &catcache[relNum];

    if (entry->relFile < 0)
    {
        db_err_code = REL_OPEN_ERROR;
        return NOTOK;
    }

    if(pid<0 || pid>=(entry->relcat_rec).numPgs)
    {
        db_err_code = PAGE_OUT_OF_BOUNDS;
        return NOTOK;
    }
    
    if (buf->pid == pid)
    {
        return OK;
    }

    if (buf->dirty)
    {
        int res = FlushPage(relNum);
        if (res != OK)
            return res;
    }

    off_t offset = (off_t)pid * PAGESIZE;

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

    buf->pid = pid;
    buf->dirty = false;

    if(debug_flag)
    {
        // printf("[DEBUG] Read page %d of relation '%s' into buffer slot %d\n", pid, entry->relcat_rec.relName, relNum);
    }
    
    return OK;
}
