#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stddef.h>
#include <limits.h>
#include <time.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/findrec.h"
#include "../include/closerel.h"


int OpenRel(const char *relName)
{
    // Step 1: Check if already open
    int relNum = FindRelNum(relName);
    bool found = false;
    
    if (relNum != NOTOK)
    {
        catcache[relNum].timestamp = (uint32_t)time(NULL);
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

    if(freeSlot == -1)
    {
        uint32_t minTimeStamp = UINT_MAX;

        for(int i = 2; i < MAXOPEN; i++)
        {
            uint32_t ts = catcache[i].timestamp;

            if(ts < minTimeStamp)
            {
                minTimeStamp = catcache[i].timestamp;
                freeSlot = i;
            }
        }

        CloseRel(freeSlot);
    }

    Rid startRid = (Rid){-1, -1};
    int relCatRecSize = catcache[RELCAT_CACHE].relcat_rec.recLength;
    RelCatRec rc;
    FindRec(RELCAT_CACHE, startRid, &startRid, &rc, 's', RELNAME, offsetof(RelCatRec, relName), (void *)relName, CMP_EQ);

    if(!isValidRid(startRid))
    {
        db_err_code = RELNOEXIST;
        return NOTOK;
    }
    else
    {
        found = true;
        catcache[freeSlot].relcat_rec = rc;
        catcache[freeSlot].relFile = open(rc.relName, O_RDWR);
        catcache[freeSlot].status = VALID_MASK;
        catcache[freeSlot].relcatRid = startRid;
        catcache[freeSlot].attrList = NULL; 
    }

    AttrDesc *ptr = NULL;
    AttrDesc **head = &(catcache[freeSlot].attrList);
    int attrCatRecSize = catcache[ATTRCAT_CACHE].relcat_rec.recLength;
    AttrCatRec ac;

    startRid = INVALID_RID;
    do
    {
        FindRec(ATTRCAT_CACHE, startRid, &startRid, &ac, 's', ATTRNAME, offsetof(AttrCatRec, relName), (void *)relName, CMP_EQ);
        
        if(isValidRid(startRid))
        {
            void *newNodePtr = malloc(sizeof(AttrDesc)); 
            
            if(!newNodePtr)
            {
                db_err_code = MEM_ALLOC_ERROR;
                FreeLinkedList(head, offsetof(AttrDesc, next));
                return NOTOK;
            }

            if(!*head)
            {
                *head = newNodePtr;
                (*head)->attr = ac;
                (*head)->next = NULL;
                ptr = *head;
            }
            else
            {
                ptr->next = newNodePtr;
                ptr = ptr->next;
                ptr->attr = ac;
                ptr->next = NULL;
            }
        }
        else
        {
            break;
        }
    }
    while(1);
    
    if(found)
    {
        catcache[freeSlot].timestamp = (uint32_t)time(NULL);
        return freeSlot;
    }
    else
    {
        db_err_code = RELNOEXIST;
        return NOTOK;
    }
}
