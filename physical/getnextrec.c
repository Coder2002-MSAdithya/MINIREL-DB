#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/readpage.h"

int GetNextRec (int relNum, Rid *startRid, Rid *foundRid, char* recPtr)
{
    if(!(catcache[relNum].status & VALID_MASK))
    {
        db_err_code = INVALID_RELNUM;
        return NOTOK;
    }

    int recSize = catcache[relNum].relcat_rec.recLength;
    int recsPerPg = catcache[relNum].relcat_rec.recsPerPg;
    int numPages = catcache[relNum].relcat_rec.numPgs;
    char *page = buffer[relNum].page;
    foundRid = NULL;

    //Check magic
    if(strncmp(page+1, GEN_MAGIC, MAGIC_SIZE-1))
    {
        db_err_code = PAGE_MAGIC_ERROR;
        return NOTOK;
    }

    //Extract slotmap (8 bytes after magic)
    unsigned long slotmap = 0;
    memcpy(&slotmap, page + MAGIC_SIZE, SLOTMAP);

    if (!startRid)
    {
        ReadPage(relNum, 0);
        foundRid->pid = 0;
        foundRid->slotnum = 0;
        memcpy(recPtr, page + HEADER_SIZE, recSize);
    }

    else
    {
        int nextslot = (startRid->slotnum) + 1;
        short pageId = startRid->pid;
        int offset = HEADER_SIZE + nextslot*recSize;

        while(!foundRid)
        {
            ReadPage(relNum, pageId);
            memcpy(&slotmap, page + MAGIC_SIZE, SLOTMAP);
            
            while(!(slotmap & nextslot << 1UL) && (nextslot <= recsPerPg))
            {
                nextslot += 1;
            }
            if (nextslot <= recsPerPg)
            {
                offset = HEADER_SIZE + nextslot*recSize;
                memcpy(recPtr, page + offset, recSize);
                foundRid->pid = pageId;
                foundRid->slotnum = nextslot;
            }
            else
            {
                pageId = pageId+1;
            }

        }
    }
    
    return OK;
}
