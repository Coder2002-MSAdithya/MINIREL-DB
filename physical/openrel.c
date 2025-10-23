#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/readpage.h"


int OpenRel(const char *relName)
{
    // Step 1: Check if already open
    int relNum = FindRelNum(relName);
    
    if (relNum != NOTOK)
    {
        return relNum;  // Already open
    }

    // Step 2: Find free cache slot
    int freeSlot = -1;
    for (int i = 2; i < MAXOPEN; i++)
    {
        if (!(catcache[i].status & VALID_MASK))  // Empty slot
        {
            freeSlot = i;
            break;
        }
    }

    //TODO : Find victim and do cache replacement here
    if(freeSlot == -1)
    {
        db_err_code = BUFFER_FULL;
        return NOTOK;
    }

    //Step 3: Loop through each record and page of relcat relation
    int np = catcache[RELCAT_CACHE].relcat_rec.numPgs;
    bool found = false;
    RelCatRec rc;

    for(short pidx = 0; pidx < np; pidx++)
    {
        if(ReadPage(RELCAT_CACHE, pidx) != OK)
        {
            db_err_code = REL_OPEN_ERROR;
            return NOTOK;
        }
        
        char *page = buffer[RELCAT_CACHE].page;

        //Check magic
        if(strncmp(page, "$MINIREL", MAGIC_SIZE))
        {
            db_err_code = PAGE_MAGIC_ERROR;
            return NOTOK;
        }

        //Extract slotmap (8 bytes after magic)
        unsigned long slotmap = 0;
        memcpy(&slotmap, page + MAGIC_SIZE, sizeof(slotmap));

        int recSize = catcache[RELCAT_CACHE].relcat_rec.recLength;
        int recsPerPg = catcache[RELCAT_CACHE].relcat_rec.recsPerPg;

        // Loop through each slot
        for(short slot=0; slot<recsPerPg; slot++)
        {
            // If bit 'slot' is set (LSB-first means bit 0 = slot 0)
            if((slotmap & (1UL << slot)))
            {
                int offset = HEADER_SIZE + slot * recSize;
                memcpy(&rc, page + offset, recSize);

                if(strncmp(rc.relName, relName, RELNAME) == 0)
                {
                    found = true;
                    catcache[freeSlot].relcat_rec = rc;
                    catcache[freeSlot].relFile = open(relName, O_RDWR);
                    catcache[freeSlot].status = VALID_MASK;
                    catcache[freeSlot].relcatRid.pid = pidx;
                    catcache[freeSlot].relcatRid.slotnum = slot;
                    catcache[freeSlot].attrList = NULL;
                    if(found) break;
                }
            }
        }

        if(found) break;
    }

    // Now loop through each record of attrcat and add columns with matching relName in the attrList
    np = catcache[ATTRCAT_CACHE].relcat_rec.numPgs;
    AttrCatRec ac;

    //Prepare to create Linked list of attribute descriptors
    AttrDesc *ptr = NULL;
    AttrDesc **head = &(catcache[freeSlot].attrList);

    for(short pidx = 0; pidx < np; pidx++)
    {
        if(ReadPage(ATTRCAT_CACHE, pidx) != OK)
        {
            db_err_code = REL_OPEN_ERROR;
            return NOTOK;
        }

        char *page = buffer[ATTRCAT_CACHE].page;

        //Check magic
        if(strncmp(page, "!MINIREL", MAGIC_SIZE))
        {
            db_err_code = PAGE_MAGIC_ERROR;
            return NOTOK;
        }

        //Extract slotmap (8 bytes after magic)
        unsigned long slotmap = 0;
        memcpy(&slotmap, page + MAGIC_SIZE, sizeof(slotmap));

        int recSize = catcache[ATTRCAT_CACHE].relcat_rec.recLength;
        int recsPerPg = catcache[ATTRCAT_CACHE].relcat_rec.recsPerPg;

        // Loop through each slot
        for(short slot=0; slot < recsPerPg; slot++)
        {
            if(slotmap & (1UL << slot))
            {
                int offset = HEADER_SIZE + slot * recSize;
                memcpy(&ac, page + offset, recSize);
                
                if(strncmp(ac.relName, relName, RELNAME) == 0)
                {
                    if(!*head)
                    {
                        *head = malloc(sizeof(AttrDesc));
                        (*head)->attr = ac;
                        (*head)->next = NULL;
                        ptr = *head;
                    }
                    else
                    {
                        ptr->next = malloc(sizeof(AttrDesc));
                        ptr = ptr->next;
                        ptr->attr = ac;
                        ptr->next = NULL;
                    }
                }
            }
        }
    }

    if(found)
    {
        return freeSlot;
    }

    db_err_code = RELNOEXIST;
    return NOTOK;
}
