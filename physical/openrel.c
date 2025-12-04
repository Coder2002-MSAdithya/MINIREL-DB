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
#include "../include/pinrel.h"
#include "../include/unpinrel.h"

/*------------------------------------------------------------

FUNCTION OpenRel (relName)

PARAMETER DESCRIPTION:
    relName  → Name of the relation whose heap file and catalog information must be loaded into an open-cache slot.

FUNCTION DESCRIPTION:
    This routine is responsible for making a relation available for tuple-level operations. 
    It ensures the relation’s metadata (RelCatRec + its AttrCatRec linked list) are loaded into an available catcache slot, and its file is opened for R/W.
    If the relation is already open, its timestamp is refreshed and the corresponding relation number is returned.
    If the cache is full, the least-recently-used relation (determined via timestamps) is closed and reused.

ALGORITHM:
    1) Check if the relation is already open using FindRelNum().
        If yes, update timestamp and return cached relNum.
    2) Search for a free cache slot among slots [2..MAXOPEN-1]. Slots 0 and 1 are for catalog relations.)
    3) If no free slot exists, select a victim slot using an LRU policy: the slot with the smallest timestamp.
    4) Ensure the victim slot is closed via CloseRel().
    5) Lookup the relation in the relcat using FindRec().
        If not present, return RELNOEXIST.
    6) Open the associated file via open().
    7) Copy the retrieved RelCatRec into the selected cache slot and set:
        - relFile descriptor,
        - status = VALID_MASK,
        - relcatRid for future catalog writes,
        - timestamp for LRU bookkeeping.
    8) Build the linked list of attribute descriptors for this relation by repeatedly scanning attrcat using FindRec() on attrCat.relName = relName, and allocate AttrDesc nodes.
    9) Return the cache slot index.

BUGS:
    None found.

ERRORS REPORTED:
    RELNOEXIST         – relation not in relcat
    FILESYSTEM_ERROR   – File could not be opened
    MEM_ALLOC_ERROR    – failed to allocate AttrDesc node

GLOBAL VARIABLES MODIFIED:
    catcache[]         – new cache entry created/updated
    db_err_code        – set on error
    buffer[]           – indirectly affected when CloseRel evicts pages

------------------------------------------------------------*/

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

            if(ts < minTimeStamp && !(catcache[i].status & PINNED_MASK))
            {
                minTimeStamp = catcache[i].timestamp;
                freeSlot = i;
            }
        }
    }

    if(freeSlot == -1)
    {
        db_err_code == BUFFER_FULL;
        return NOTOK;
    }

    Rid startRid = (Rid){-1, -1};
    int relCatRecSize = catcache[RELCAT_CACHE].relcat_rec.recLength;
    RelCatRec rc;

    if(FindRec(RELCAT_CACHE, startRid, &startRid, &rc, 's', RELNAME, offsetof(RelCatRec, relName), (void *)relName, CMP_EQ) == NOTOK)
    {
        return NOTOK;
    }

    if(!isValidRid(startRid))
    {
        db_err_code = RELNOEXIST;
        return NOTOK;
    }

    found = true;
    if(CloseRel(freeSlot) == NOTOK)
    {
        return NOTOK;
    }

    printf("Victim slot %d chosen for relation %s of DB.\n", freeSlot, relNum);
    PinRel(freeSlot);

    int fd = open(rc.relName, O_RDWR);

    if(fd == NOTOK)
    {
        db_err_code = FILESYSTEM_ERROR;
        return NOTOK;
    }

    catcache[freeSlot].relcat_rec = rc;
    catcache[freeSlot].relFile = fd;
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
                (*head)->attrCatRid = startRid;
                (*head)->next = NULL;
                ptr = *head;
            }
            else
            {
                ptr->next = newNodePtr;
                ptr = ptr->next;
                ptr->attr = ac;
                ptr->attrCatRid = startRid;
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