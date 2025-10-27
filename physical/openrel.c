#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stddef.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/findrec.h"


int OpenRel(const char *relName)
{
    // Step 1: Check if already open
    int relNum = FindRelNum(relName);
    bool found = false;
    
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

    Rid startRid = (Rid){-1, -1};
    int relCatRecSize = catcache[RELCAT_CACHE].relcat_rec.recLength;
    RelCatRec rc;
    FindRec(RELCAT_CACHE, startRid, &startRid, &rc, 's', RELNAME, offsetof(RelCatRec, relName), (void *)relName, CMP_EQ);

    if(startRid.pid < 0 || startRid.slotnum < 0)
    {
        db_err_code = RELNOEXIST;
        return NOTOK;
    }
    else
    {
        found = true;
    }

    AttrDesc *ptr = NULL;
    AttrDesc **head = &(catcache[freeSlot].attrList);
    int attrCatRecSize = catcache[ATTRCAT_CACHE].relcat_rec.recLength;
    AttrCatRec ac;

    startRid = (Rid){-1, -1};
    do
    {
        FindRec(ATTRCAT_CACHE, startRid, &startRid, &ac, 's', ATTRNAME, offsetof(AttrCatRec, relName), (void *)relName, CMP_EQ);
        
        if(startRid.pid >= 0 && startRid.slotnum >= 0)
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
        else
        {
            break;
        }
    }
    while(1);
    
    if(found)
    {
        return freeSlot;
    }
    else
    {
        db_err_code = RELNOEXIST;
        return NOTOK;
    }
}
