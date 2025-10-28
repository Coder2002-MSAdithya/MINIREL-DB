#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "../include/globals.h"
#include "../include/error.h"
#include "../include/readpage.h"
#include "../include/writerec.h"

int InsertRec(int relNum, void *recPtr)
{
    CacheEntry *entry = &catcache[relNum];

    if(!(entry->status & VALID_MASK))
    {
        db_err_code = INVALID_RELNUM;
        return NOTOK;
    }

    int recSize = (entry->relcat_rec).recLength;
    int recsPerPg = (entry->relcat_rec).recsPerPg;
    int numPages = (entry->relcat_rec).numPgs;

    bool foundFreeSlot = false;
    char *page = buffer[relNum].page;

    for(short pidx = 0; pidx < numPages; pidx++)
    {
        if(ReadPage(relNum, pidx) == NOTOK)
        {
            db_err_code = REL_OPEN_ERROR;
            return NOTOK;
        }

        //Check magic
        if(strncmp(page+1, GEN_MAGIC, MAGIC_SIZE-1))
        {
            db_err_code = PAGE_MAGIC_ERROR;
            return NOTOK;
        }

        //Extract slotmap (8 bytes after magic)
        unsigned long slotmap = 0;
        memcpy(&slotmap, page + MAGIC_SIZE, SLOTMAP);

        // Loop through each slot
        for(short slot=0; slot<recsPerPg; slot++)
        {
            // If bit 'slot' is set (LSB-first means bit 0 = slot 0)
            if(!(slotmap & (1UL << slot)))
            {
                foundFreeSlot = true;
                int offset = HEADER_SIZE + slot * recSize;
                memcpy(page + offset, recPtr, recSize);
                slotmap |= (1UL << slot);
                memcpy(page + MAGIC_SIZE, &slotmap, SLOTMAP);
                buffer[relNum].dirty = true;
                (entry->relcat_rec).numRecs += 1;
                entry->status |= DIRTY_MASK;
                WriteRec(RELCAT_CACHE, &(entry->relcat_rec), entry->relcatRid);
                return OK;
            }
        }
    }
    
    if(!foundFreeSlot)
    {
        // Create a new page here
        buffer[relNum].pid = numPages;
        buffer[relNum].dirty = true;
        memset(page, 0, PAGESIZE);
        char c = relNum == 0 ? '$' : (relNum == 1 ? '!' : '_');
        unsigned long newMap = 1UL;
        page[0] = c;
        strncpy(page+1, GEN_MAGIC, MAGIC_SIZE-1);
        memcpy(page+MAGIC_SIZE, &newMap, SLOTMAP);
        memcpy(page+HEADER_SIZE, recPtr, recSize);
        (entry->relcat_rec).numRecs += 1;
        (entry->relcat_rec).numPgs += 1;
        entry->status |= DIRTY_MASK;
        WriteRec(RELCAT_CACHE, &(entry->relcat_rec), entry->relcatRid);
        return OK;
    }


    return NOTOK;
}
