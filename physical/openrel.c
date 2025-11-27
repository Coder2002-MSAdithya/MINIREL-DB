/************************INCLUDES*******************************/
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


/*------------------------------------------------------------

FUNCTION OpenRel (argc, argv)

PARAMETER DESCRIPTION:
    relName  →  Name of the relation to be opened.

FUNCTION DESCRIPTION:
    Opens a relation into the catalog cache if it is not already open.
    If the relation is already open, simply updates its timestamp and returns the existing cache index.
    If not open, it locates a free cache slot. 
    If the cache is full, it selects the least-recently-used (LRU) slot based on timestamps.
    Then it reads the relation's metadata from relcat via FindRec(), loads attribute metadata from attrcat, and initializes a new cache entry.

ALGORITHM:
    1. Check if relName is already open via FindRelNum():
        - If yes, update timestamp and return its index.
    2. Search for a free cache entry in slots 2..MAXOPEN-1.
    3. If no free slot exists, choose LRU entry by minimum timestamp.
    4. Search relcat for relName:
        - Use FindRec() with comparison on relName field.
        - If not found, return RELNOEXIST.
    5. CloseRel() on selected replacement slot to clean it.
    6. Load RelCatRec of relName into selected slot:
        - Set file descriptor with open().
        - Copy relcat RID.
        - Initialize attrList to empty.
    7. Scan attrcat for all attributes whose relName matches:
        - For each match, allocate a new AttrDesc node.
        - Link nodes to build attribute list.
        - On memory failure, free entire list and return NOTOK.
    8. Update timestamp of new entry.
    9. Return slot index.

ERRORS REPORTED:
    - RELNOEXIST when relation doesn't exist in relcat.
    - MEM_ALLOC_ERROR on malloc() failure.
    - Does not set db_err_code for file open failures or LRU closing failures.

GLOBAL VARIABLES MODIFIED:
    - catcache[slot] entries (metadata, fd, timestamp, attrList).
    - db_err_code on error paths.

------------------------------------------------------------*/

int OpenRel(const char *relName)
{
    int relNum = FindRelNum(relName);
    bool found = false;
    
    if (relNum != NOTOK)
    {
        catcache[relNum].timestamp = (uint32_t)time(NULL);
        return relNum;
    }

    int freeSlot = -1;
    for (int i = 2; i < MAXOPEN; i++)
    {
        if (!(catcache[i].status & VALID_MASK)) 
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

    found = true;

    if(CloseRel(freeSlot)!=OK)
    {
        return NOTOK;
    }

    catcache[freeSlot].relFile = open(rc.relName, O_RDWR);
    if(catcache[freeSlot].relFile<0)
    {
        return NOTOK;
    }
    catcache[freeSlot].relcat_rec = rc;
    catcache[freeSlot].status = VALID_MASK;
    catcache[freeSlot].relcatRid = startRid;
    catcache[freeSlot].attrList = NULL; 

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
                FreeLinkedList((void **)head, offsetof(AttrDesc, next));
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
