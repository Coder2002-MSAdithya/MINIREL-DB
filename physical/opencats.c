/************************INCLUDES*******************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <limits.h>
#include "../include/defs.h"
#include "../include/error.h"
#include "../include/globals.h"
#include "../include/helpers.h"
#include "../include/readpage.h"


/*------------------------------------------------------------

FUNCTION BuildAttrList (attrArray, numAttrs)

PARAMETER DESCRIPTION:
    attrArray → array of pointers to AttrCatRec structures
    numAttrs  → number of attributes in the array

FUNCTION DESCRIPTION:
    Constructs a linked list of AttrDesc nodes from an array of attribute-catalog records.  
    Each AttrDesc node is populated by copying the AttrCatRec pointed to by attrArray[i].  
    The nodes are linked in the same order as provided by the caller.
    On memory-allocation failure, the partially constructed list is freed and NULL is returned.

ALGORITHM:
    1) Initialize head and tail pointers to NULL.
    2) For each element in attrArray:
        a) Allocate a new AttrDesc node.
        b) Copy the AttrCatRec into node->attr.
        c) Set node->next = NULL.
        d) Append to the linked list (update head/tail).
    3) If all allocations succeed, return the head pointer.
    4) If any allocation fails:
        – free the entire list using FreeLinkedList(),
        – set error code MEM_ALLOC_ERROR,
        – return NULL.

BUGS:
    None found.

ERRORS REPORTED:
    MEM_ALLOC_ERROR (if malloc fails)

GLOBAL VARIABLES MODIFIED:
    db_err_code (set on allocation failure)

------------------------------------------------------------*/

AttrDesc* BuildAttrList(AttrCatRec **attrArray, int numAttrs)
{
    AttrDesc *head=NULL, *tail=NULL;
    for(int i=0;i<numAttrs;i++)
    {
        AttrDesc *node=malloc(sizeof(AttrDesc));
        if(!node)
        {
            db_err_code = MEM_ALLOC_ERROR;
            FreeLinkedList((void **)&head, offsetof(AttrDesc, next));
            return NULL;
        }
        node->attr=*attrArray[i];
        node->next=NULL;

        if(!head)
        {
            head=node;
            tail=node;
        }
        else
        {
            tail->next=node;
            tail=node;
        }
    }
    return head;
}


/*------------------------------------------------------------

FUNCTION OpenCats ()

PARAMETER DESCRIPTION:
    None

FUNCTION DESCRIPTION:
    Loads the relation catalog (relcat) and attribute catalog (attrcat) into the in-memory cache structures.  
    This routine is invoked once when a database is opened and prepares MINIREL's global metadata so that future relational operators can access schema information.
    Steps performed:
        • Opens relcat and attrcat heap files.
        • Reads first page of relcat to extract RelCatRec entries describing relcat and attrcat themselves.
        • Constructs AttrDesc linked lists for each catalog using BuildAttrList().
        • Initializes catcache[0] and catcache[1] to hold the catalog schemas.
        • Clears all buffers in the buffer pool.

ALGORITHM:
    1) Attempt to open relcat and attrcat with read/write access.
    2) Read page 0 of relcat into a temporary page buffer.
    3) Extract:
        Relcat_rc ← record 0 (relcat metadata)
        Relcat_ac ← record 1 (attrcat metadata)
    4) Build attribute lists:
        catcache[0].attrList ← BuildAttrList(rel_attrs,…)
        catcache[1].attrList ← BuildAttrList(attr_attrs,…)
    5) Populate cache entries for relcat (slot 0) and attrcat (slot 1):
        – relFile = file descriptor
        – relcat_rec = metadata record
        – relcatRid = {pid=0, slotnum=0 or 1}
        – status = PINNED_MASK | VALID_MASK
    6) Zero-initialize the entire buffer pool:
        pid = -1, dirty = 0, page[] = 0
    7) Mark db_open = true.
    8) Return OK.

ERRORS REPORTED:
    FILESYSTEM_ERROR (if opening or reading catalog files fails)
    MEM_ALLOC_ERROR  (propagated from BuildAttrList failures)

GLOBAL VARIABLES MODIFIED:
    catcache[] entries 0 and 1
    buffer[] (buffer pool reset)
    db_open

------------------------------------------------------------*/

int OpenCats()
{
    // Open catalog files
    int rel_fd = open(RELCAT, O_RDWR);
    int attr_fd = open(ATTRCAT, O_RDWR);

    if(rel_fd == NOTOK || attr_fd == NOTOK)
    {
        db_err_code = FILESYSTEM_ERROR;
        return NOTOK;
    }

    // Page variable
    char page[PAGESIZE];

    // To place the current catalog values of relcat and attrcat
    RelCatRec Relcat_rc, Relcat_ac;

    // Read the first page of relcat into page buffer
    if(read(rel_fd, page, PAGESIZE) == NOTOK)
    {
        db_err_code = FILESYSTEM_ERROR;
        return NOTOK;
    }

    // The first record of relcat relation stores relcat's info
    memcpy(&Relcat_rc, page+HEADER_SIZE, sizeof(RelCatRec));

    // The second record of the relcat relation holds attrcat's info
    memcpy(&Relcat_ac, page+HEADER_SIZE+sizeof(RelCatRec), sizeof(RelCatRec));

    // Attributes of relcat relation
    AttrCatRec *rel_attrs[] = {
        &Attrcat_rrelName,
        &Attrcat_recLength,
        &Attrcat_recsPerPg,
        &AttrCat_numAttrs,
        &AttrCat_numRecs,
        &AttrCat_numPgs
    };

    // Attributes of attrcat relation
    AttrCatRec *attr_attrs[] = {
        &AttrCat_offset,
        &AttrCat_length,
        &AttrCat_type,
        &AttrCat_attrName,
        &AttrCat_arelName,
        &AttrCat_hasIndex,
        &AttrCat_nPages,
        &AttrCat_nKeys
    };

    // Load relcat entry into cache[0]
    catcache[0].relcat_rec = Relcat_rc;
    catcache[0].relFile = rel_fd;
    catcache[0].status = (PINNED_MASK | VALID_MASK); // last three bits are 1 (for protected), 1 (for valid) and 0 (for clean)
    /* Record for attrcat is in slot 0 of the page 0 (O based indexing of pages and slots)*/
    catcache[0].relcatRid.pid = 0;
    catcache[0].relcatRid.slotnum = 0;
    catcache[0].attrList = BuildAttrList(rel_attrs, RELCAT_NUMATTRS);

    // Load attrcat entry into cache[1]
    catcache[1].relcat_rec = Relcat_ac;
    catcache[1].relFile = attr_fd;
    catcache[1].status = (PINNED_MASK | VALID_MASK); // last three bits are 1 (for metadata), 1 (for valid) and 0 (for clean)
    /* Record for attrcat is in slot 1 of the page 0 (O based indexing of pages and slots)*/
    catcache[1].relcatRid.pid = 0;
    catcache[1].relcatRid.slotnum = 1;
    catcache[1].attrList = BuildAttrList(attr_attrs, ATTRCAT_NUMATTRS);

    // Initialize buffer pool
    for (int i = 0; i < MAXOPEN; i++) 
    {
        buffer[i].dirty = 0;
        buffer[i].pid = -1;
        memset(buffer[i].page, 0, sizeof(buffer[i].page));
    }

    // Mark DB as open (OpenDB already does this)
    db_open = true;

    return OK;
}
