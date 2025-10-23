#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"


int CloseRel(int relNum)
{
    if (relNum < 0 || relNum >= MAXOPEN)
        return NOTOK;

    CacheEntry *entry = &catcache[relNum];

    if (!(entry->status & VALID_MASK))
        return NOTOK; // Not open

    // Step 1: Flush dirty page if any
    if(buffer[relNum].dirty)
    {
        FlushPage(relNum);
    }

    // Step 2: If catalog info dirty, update relcat on disk
    if ((entry->status) & DIRTY_MASK)
    {
        FILE *fp = fopen(RELCAT, "r+b");

        if(!fp)
        {
            db_err_code = REL_CLOSE_ERROR;
            return NOTOK;
        }

        short pid = catcache[relNum].relcatRid.pid;
        short slotnum = catcache[relNum].relcatRid.slotnum;

        fseek(fp, pid * PAGESIZE + HEADER_SIZE + slotnum * sizeof(RelCatRec), SEEK_SET);
        fwrite(&(entry->relcat_rec), sizeof(RelCatRec), 1, fp);
        fclose(fp);
    }

    // Step 3: Close file
    close(entry->relFile);

    // Step 4: Clear cache + buffer
    memset(entry, 0, sizeof(CacheEntry));
    memset(&buffer[relNum], 0, sizeof(Buffer));

    return OK;
}